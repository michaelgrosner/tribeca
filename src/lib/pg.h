#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  json pgPos;
  json pgSafety;
  map<double, json> pgBuys;
  map<double, json> pgSells;
  map<int, json> pgWallet;
  vector<json> pgProfit;
  double pgTargetBasePos = 0;
  string pgSideAPR = "";
  string pgSideAPR_ = "!=";
  class PG {
    public:
      static void main(Local<Object> exports) {
        load();
        EV::evOn("PositionGateway", [](json k) {
          posUp(k);
        });
        EV::evOn("FairValue", [](json k) {
          posUp({});
        });
        EV::evOn("OrderUpdateBroker", [](json k) {
          orderUp(k);
        });
        EV::evOn("QuotingParameters", [](json k) {
          calcTargetBasePos();
          if (mgFairValue) calcSafety();
        });
        EV::evOn("PositionBroker", [](json k) {
          calcTargetBasePos();
        });
        EV::evOn("OrderTradeBroker", [](json k) {
          tradeUp(k);
          if (mgFairValue) calcSafety();
        });
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        UI::uiSnap(uiTXT::TradeSafetyValue, &onSnapSafety);
        UI::uiSnap(uiTXT::TargetBasePosition, &onSnapTargetBasePos);
      };
      static void calc() {
        if (pgPos.is_null()) return;
        json safety = calcSafety();
        if (pgSafety.is_null()
          or abs(safety["combined"].get<double>() - pgSafety["combined"].get<double>()) > 1e-3
          or abs(safety["buyPing"].get<double>() - pgSafety["buyPing"].get<double>()) >= 1e-2
          or abs(safety["sellPong"].get<double>() - pgSafety["sellPong"].get<double>()) >= 1e-2
        ) {
          pgSafety = safety;
          EV::evUp("Safety");
          UI::uiSend(uiTXT::TradeSafetyValue, safety, true);
        }
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::TargetBasePosition);
        if (k.size()) {
          if (k["/0/tbp"_json_pointer].is_number())
            pgTargetBasePos = k["/0/tbp"_json_pointer].get<double>();
          if (k["/0/sideAPR"_json_pointer].is_string())
            pgSideAPR = k["/0/sideAPR"_json_pointer].get<string>();
        }
        cout << FN::uiT() << "TBP = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << " loaded from DB." << endl;
      };
      static json onSnapPos(json z) {
        return { pgPos };
      };
      static json onSnapSafety(json z) {
        return { pgSafety };
      };
      static json onSnapTargetBasePos(json z) {
        return {{
          {"tbp", pgTargetBasePos},
          {"sideAPR", pgSideAPR}
        }};
      };
      static json calcSafety() {
        double buySize = qpRepo["percentageValues"].get<bool>()
          ? qpRepo["buySizePercentage"].get<double>() * pgPos["value"].get<double>() / 100
          : qpRepo["buySize"].get<double>();
        double sellSize = qpRepo["percentageValues"].get<bool>()
          ? qpRepo["sellSizePercentage"].get<double>() * pgPos["value"].get<double>() / 100
          : qpRepo["sellSize"].get<double>();
        double totalBasePosition = pgPos["baseAmount"].get<double>() + pgPos["baseHeldAmount"].get<double>();
        if (qpRepo["buySizeMax"].get<bool>() and (mAPR)qpRepo["aggressivePositionRebalancing"].get<int>() != mAPR::Off)
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if (qpRepo["sellSizeMax"].get<bool>() and (mAPR)qpRepo["aggressivePositionRebalancing"].get<int>() != mAPR::Off)
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        double widthPong = qpRepo["widthPercentage"].get<bool>()
          ? qpRepo["widthPongPercentage"].get<double>() * mgFairValue / 100
          : qpRepo["widthPong"].get<double>();
        map<double, json> tradesBuy;
        map<double, json> tradesSell;
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          if ((mSide)(*it)["side"].get<int>() == mSide::Bid)
            tradesBuy[(*it)["price"].get<double>()] = *it;
          else tradesSell[(*it)["price"].get<double>()] = *it;
        double buyPing = 0;
        double sellPong = 0;
        double buyQty = 0;
        double sellQty = 0;
        if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingFair
          or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingAggressive
        ) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong*-1);
        } else if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingFair
          or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingAggressive
        ) {
          matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchLastPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong, true);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPong /= sellQty;
        clean();
        double sumBuys = sum(&pgBuys);
        double sumSells = sum(&pgSells);
        return {
          {"buy", sumBuys / buySize},
          {"sell", sumSells / sellSize},
          {"combined", (sumBuys + sumSells) / (buySize + sellSize)},
          {"buyPing", buyPing},
          {"sellPong", sellPong}
        };
      };
      static void matchFirstPing(map<double, json>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchBestPing(map<double, json>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchLastPing(map<double, json>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchPing(bool matchPings, bool near, bool far, map<double, json>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<double, json>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second["price"].get<double>(), it->second["quantity"].get<double>(), it->second["price"].get<double>(), it->second["Kqty"].get<double>(), reverse))
            break;
        } else for (map<double, json>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second["price"].get<double>(), it->second["quantity"].get<double>(), it->second["price"].get<double>(), it->second["Kqty"].get<double>(), reverse))
            break;
      };
      static bool matchPing(bool matchPings, bool near, bool far, double *ping, double width, double* qty, double qtyMax, double fv, double price, double qtyTrade, double priceTrade, double KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (*qty < qtyMax
          and (far ? fv > price : true)
          and (near ? (reverse ? fv - width : fv + width) < price : true)
          and (!matchPings or KqtyTrade < qtyTrade)
        ) matchPing(ping, qty, qtyMax, qtyTrade, priceTrade);
        return *qty >= qtyMax;
      };
      static void matchPing(double* ping, double* qty, double qtyMax, double qtyTrade, double priceTrade) {
        double qty_ = fmin(qtyMax - *qty, qtyTrade);
        *ping += priceTrade * qty_;
        *qty += qty_;
      }
      static void tradeUp(json k) {
        json k_ = {
          {"price", k["price"].get<double>()},
          {"quantity", k["quantity"].get<double>()},
          {"time", k["time"].get<unsigned long>()}
        };
        if ((mSide)k["side"].get<int>() == mSide::Bid)
          pgBuys[k["price"].get<double>()] = k_;
        else pgSells[k["price"].get<double>()] = k_;
      };
      static void clean() {
        if (pgBuys.size()) expire(&pgBuys);
        if (pgSells.size()) expire(&pgSells);
        skip();
      };
      static void expire(map<double, json>* k) {
        unsigned long now = FN::T();
        for (map<double, json>::iterator it = k->begin(); it != k->end();)
          if (it->second["time"].get<unsigned long>() + qpRepo["tradeRateSeconds"].get<double>() * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      static void skip() {
        while (pgBuys.size() and pgSells.size()) {
          json buy = pgBuys.rbegin()->second;
          json sell = pgSells.begin()->second;
          if (sell["price"].get<double>() < buy["price"].get<double>()) break;
          double buyQty = buy["quantity"].get<double>();
          buy["quantity"] = buyQty - sell["quantity"].get<double>();
          sell["quantity"] = sell["quantity"].get<double>() - buyQty;
          if (buy["quantity"].get<double>() < gw->minSize)
            pgBuys.erase(--pgBuys.rbegin().base());
          if (sell["quantity"].get<double>() < gw->minSize)
            pgSells.erase(pgSells.begin());
        }
      };
      static double sum(map<double, json>* k) {
        double sum = 0;
        for (map<double, json>::iterator it = k->begin(); it != k->end(); ++it)
          sum += it->second["quantity"].get<double>();
        return sum;
      };
      static void posUp(json k) {
        if (!k.is_null()) pgWallet[k["currency"].get<int>()] = k;
        if (!mgFairValue or pgWallet.find(gw->base) == pgWallet.end() or pgWallet.find(gw->quote) == pgWallet.end() or pgWallet[gw->base].is_null() or pgWallet[gw->quote].is_null()) return;
        double baseAmount = pgWallet[gw->base]["amount"].get<double>();
        double quoteAmount = pgWallet[gw->quote]["amount"].get<double>();
        double baseValue = baseAmount + quoteAmount / mgFairValue + pgWallet[gw->base]["heldAmount"].get<double>() + pgWallet[gw->quote]["heldAmount"].get<double>() / mgFairValue;
        double quoteValue = baseAmount * mgFairValue + quoteAmount + pgWallet[gw->base]["heldAmount"].get<double>() * mgFairValue + pgWallet[gw->quote]["heldAmount"].get<double>();
        unsigned long now = FN::T();
        pgProfit.push_back({
          {"baseValue", baseValue},
          {"quoteValue", quoteValue},
          {"time", now }
        });
        for (vector<json>::iterator it = pgProfit.begin(); it != pgProfit.end();)
          if ((*it)["time"].get<unsigned long>() + (qpRepo["profitHourInterval"].get<double>() * 36e+5) > now) ++it;
          else it = pgProfit.erase(it);
        json pos = {
          {"baseAmount", baseAmount},
          {"quoteAmount", quoteAmount},
          {"baseHeldAmount", pgWallet[gw->base]["heldAmount"].get<double>()},
          {"quoteHeldAmount", pgWallet[gw->quote]["heldAmount"].get<double>()},
          {"value", baseValue},
          {"quoteValue", quoteValue},
          {"profitBase", ((baseValue - (*pgProfit.begin())["baseValue"].get<double>()) / baseValue) * 1e+2},
          {"profitQuote", ((quoteValue - (*pgProfit.begin())["quoteValue"].get<double>()) / quoteValue) * 1e+2},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"exchange", (int)gw->exchange}
        };
        bool eq = true;
        if (!pgPos.is_null()) {
          eq = abs(pos["value"].get<double>() - pgPos["value"].get<double>()) < 2e-6;
          if(eq
            and abs(pos["quoteValue"].get<double>() - pgPos["quoteValue"].get<double>()) < 2e-2
            and abs(pos["baseAmount"].get<double>() - pgPos["baseAmount"].get<double>()) < 2e-6
            and abs(pos["quoteAmount"].get<double>() - pgPos["quoteAmount"].get<double>()) < 2e-2
            and abs(pos["baseHeldAmount"].get<double>() - pgPos["baseHeldAmount"].get<double>()) < 2e-6
            and abs(pos["quoteHeldAmount"].get<double>() - pgPos["quoteHeldAmount"].get<double>()) < 2e-2
            and abs(pos["profitBase"].get<double>() - pgPos["profitBase"].get<double>()) < 2e-2
            and abs(pos["profitQuote"].get<double>() - pgPos["profitQuote"].get<double>()) < 2e-2
          ) return;
        }
        pgPos = pos;
        if (!eq) calcTargetBasePos();
        UI::uiSend(uiTXT::Position, pos, true);
      };
      static void orderUp(json k) {
        if (pgPos.is_null()) return;
        double heldAmount = 0;
        double amount = (mSide)k["side"].get<int>() == mSide::Ask
          ? pgPos["baseAmount"].get<double>() + pgPos["baseHeldAmount"].get<double>()
          : pgPos["quoteAmount"].get<double>() + pgPos["quoteHeldAmount"].get<double>();
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (it->second["side"].get<int>() != k["side"].get<int>()) return;
          double held = it->second["quantity"].get<double>() * ((mSide)it->second["side"].get<int>() == mSide::Bid ? it->second["price"].get<double>() : 1);
          if (amount >= held) {
            amount -= held;
            heldAmount += held;
          }
        }
        posUp({
          {"amount", amount},
          {"heldAmount", heldAmount},
          {"currency", (mSide)k["side"].get<int>() == mSide::Ask
            ? k["/pair/base"_json_pointer].get<int>()
            : k["/pair/quote"_json_pointer].get<int>()}
        });
      };
      static void calcTargetBasePos() {
        if (pgPos.is_null()) { cout << FN::uiT() << "Unable to calculate TBP, missing market data." << endl; return; }
        double targetBasePosition = ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::Manual)
          ? (qpRepo["percentageValues"].get<bool>()
            ? qpRepo["targetBasePositionPercentage"].get<double>() * pgPos["value"].get<double>() / 1e+2
            : qpRepo["targetBasePosition"].get<double>())
          : ((1 + mgTargetPos) / 2) * pgPos["value"].get<double>();
        if (pgTargetBasePos and abs(pgTargetBasePos - targetBasePosition) < 1e-4 and pgSideAPR_ == pgSideAPR) return;
        pgTargetBasePos = targetBasePosition;
        pgSideAPR_ = pgSideAPR;
        EV::evUp("TargetPosition");
        json k = {{"tbp", pgTargetBasePos}, {"sideAPR", pgSideAPR}};
        UI::uiSend(uiTXT::TargetBasePosition, k, true);
        DB::insert(uiTXT::TargetBasePosition, k);
        cout << FN::uiT() << "TBP " << (int)(pgTargetBasePos / pgPos["value"].get<double>() * 1e+2) << "% = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << endl;
      };
  };
}

#endif

#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  json pgPos;
  json pgSafety;
  map<double, json> pgBuys;
  map<double, json> pgSells;
  map<int, mWallet> pgWallet;
  vector<json> pgProfit;
  double pgTargetBasePos = 0;
  string pgSideAPR = "";
  string pgSideAPR_ = "!=";
  class PG {
    public:
      static void main() {
        load();
        ev_gwDataWallet = [](mWallet k) {
          calcWallet(k);
        };
        EV::on(mEv::OrderUpdateBroker, [](json k) {
          calcWalletAfterOrder(k);
        });
        ev_mgTargetPosition = []() {
          calcTargetBasePos();
        };
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        UI::uiSnap(uiTXT::TradeSafetyValue, &onSnapSafety);
        UI::uiSnap(uiTXT::TargetBasePosition, &onSnapTargetBasePos);
      };
      static void calcSafety() {
        if (pgPos.is_null() or !mgFairValue) return;
        json safety = nextSafety();
        if (pgSafety.is_null()
          or abs(safety.value("combined", 0.0) - pgSafety.value("combined", 0.0)) > 1e-3
          or abs(safety.value("buyPing", 0.0) - pgSafety.value("buyPing", 0.0)) >= 1e-2
          or abs(safety.value("sellPong", 0.0) - pgSafety.value("sellPong", 0.0)) >= 1e-2
        ) {
          pgSafety = safety;
          UI::uiSend(uiTXT::TradeSafetyValue, safety, true);
        }
      };
      static void calcTargetBasePos() {
        if (pgPos.is_null()) { cout << FN::uiT() << "QE" << RRED << " Warrrrning:" << BRED << " Unable to calculate TBP, missing market data." << endl; return; }
        double targetBasePosition = ((mAutoPositionMode)QP::getInt("autoPositionMode") == mAutoPositionMode::Manual)
          ? (QP::getBool("percentageValues")
            ? QP::getDouble("targetBasePositionPercentage") * pgPos.value("value", 0.0) / 1e+2
            : QP::getDouble("targetBasePosition"))
          : ((1 + mgTargetPos) / 2) * pgPos.value("value", 0.0);
        if (pgTargetBasePos and abs(pgTargetBasePos - targetBasePosition) < 1e-4 and pgSideAPR_ == pgSideAPR) return;
        pgTargetBasePos = targetBasePosition;
        pgSideAPR_ = pgSideAPR;
        ev_pgTargetBasePosition();
        json k = {{"tbp", pgTargetBasePos}, {"sideAPR", pgSideAPR}};
        UI::uiSend(uiTXT::TargetBasePosition, k, true);
        DB::insert(uiTXT::TargetBasePosition, k);
        cout << FN::uiT() << "TBP " << (int)(pgTargetBasePos / pgPos.value("value", 0.0) * 1e+2) << "% = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << endl;
      };
      static void addTrade(json k) {
        json k_ = {
          {"price", k.value("price", 0.0)},
          {"quantity", k.value("quantity", 0.0)},
          {"time", k.value("time", (unsigned long)0)}
        };
        if ((mSide)k.value("side", 0) == mSide::Bid)
          pgBuys[k.value("price", 0.0)] = k_;
        else pgSells[k.value("price", 0.0)] = k_;
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::TargetBasePosition);
        if (k.size()) {
          k = k.at(0);
          pgTargetBasePos = k.value("tbp", 0.0);
          pgSideAPR = k.value("sideAPR", "");
        }
        cout << FN::uiT() << "DB" << RWHITE << " loaded TBP = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << "." << endl;
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
      static json nextSafety() {
        double buySize = QP::getBool("percentageValues")
          ? QP::getDouble("buySizePercentage") * pgPos.value("value", 0.0) / 100
          : QP::getDouble("buySize");
        double sellSize = QP::getBool("percentageValues")
          ? QP::getDouble("sellSizePercentage") * pgPos.value("value", 0.0) / 100
          : QP::getDouble("sellSize");
        double totalBasePosition = pgPos.value("baseAmount", 0.0) + pgPos.value("baseHeldAmount", 0.0);
        if (QP::getBool("buySizeMax") and (mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off)
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if (QP::getBool("sellSizeMax") and (mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off)
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        double widthPong = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPongPercentage") * mgFairValue / 100
          : QP::getDouble("widthPong");
        map<double, json> tradesBuy;
        map<double, json> tradesSell;
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          if ((mSide)it->value("side", 0) == mSide::Bid)
            tradesBuy[it->value("price", 0.0)] = *it;
          else tradesSell[it->value("price", 0.0)] = *it;
        double buyPing = 0;
        double sellPong = 0;
        double buyQty = 0;
        double sellQty = 0;
        if ((mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingFair
          or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
        ) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong*-1);
        } else if ((mPongAt)QP::getInt("pongAt") == mPongAt::LongPingFair
          or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
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
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second.value("price", 0.0), it->second.value("quantity", 0.0), it->second.value("price", 0.0), it->second.value("Kqty", 0.0), reverse))
            break;
        } else for (map<double, json>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second.value("price", 0.0), it->second.value("quantity", 0.0), it->second.value("price", 0.0), it->second.value("Kqty", 0.0), reverse))
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
      };
      static void clean() {
        if (pgBuys.size()) expire(&pgBuys);
        if (pgSells.size()) expire(&pgSells);
        skip();
      };
      static void expire(map<double, json>* k) {
        unsigned long now = FN::T();
        for (map<double, json>::iterator it = k->begin(); it != k->end();)
          if (it->second.value("time", (unsigned long)0) + QP::getDouble("tradeRateSeconds") * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      static void skip() {
        while (pgBuys.size() and pgSells.size()) {
          json buy = pgBuys.rbegin()->second;
          json sell = pgSells.begin()->second;
          if (sell.value("price", 0.0) < buy.value("price", 0.0)) break;
          double buyQty = buy.value("quantity", 0.0);
          buy["quantity"] = buyQty - sell.value("quantity", 0.0);
          sell["quantity"] = sell.value("quantity", 0.0) - buyQty;
          if (buy.value("quantity", 0.0) < gw->minSize)
            pgBuys.erase(--pgBuys.rbegin().base());
          if (sell.value("quantity", 0.0) < gw->minSize)
            pgSells.erase(pgSells.begin());
        }
      };
      static double sum(map<double, json>* k) {
        double sum = 0;
        for (map<double, json>::iterator it = k->begin(); it != k->end(); ++it)
          sum += it->second.value("quantity", 0.0);
        return sum;
      };
      static void calcWallet(mWallet k) {
        if (k.currency>-1)  pgWallet[k.currency] = k;
        if (!mgFairValue or pgWallet.find(gw->base) == pgWallet.end() or pgWallet.find(gw->quote) == pgWallet.end()) return;
        double baseAmount = pgWallet[gw->base].amount;
        double quoteAmount = pgWallet[gw->quote].amount;
        double baseValue = baseAmount + quoteAmount / mgFairValue + pgWallet[gw->base].held + pgWallet[gw->quote].held / mgFairValue;
        double quoteValue = baseAmount * mgFairValue + quoteAmount + pgWallet[gw->base].held * mgFairValue + pgWallet[gw->quote].held;
        unsigned long now = FN::T();
        pgProfit.push_back({
          {"baseValue", baseValue},
          {"quoteValue", quoteValue},
          {"time", now }
        });
        for (vector<json>::iterator it = pgProfit.begin(); it != pgProfit.end();)
          if (it->value("time", (unsigned long)0) + (QP::getDouble("profitHourInterval") * 36e+5) > now) ++it;
          else it = pgProfit.erase(it);
        json pos = {
          {"baseAmount", baseAmount},
          {"quoteAmount", quoteAmount},
          {"baseHeldAmount", pgWallet[gw->base].held},
          {"quoteHeldAmount", pgWallet[gw->quote].held},
          {"value", baseValue},
          {"quoteValue", quoteValue},
          {"profitBase", ((baseValue - (*pgProfit.begin())["baseValue"].get<double>()) / baseValue) * 1e+2},
          {"profitQuote", ((quoteValue - (*pgProfit.begin())["quoteValue"].get<double>()) / quoteValue) * 1e+2},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"exchange", (int)gw->exchange}
        };
        bool eq = true;
        if (!pgPos.is_null()) {
          eq = abs(pos.value("value", 0.0) - pgPos.value("value", 0.0)) < 2e-6;
          if(eq
            and abs(pos.value("quoteValue", 0.0) - pgPos.value("quoteValue", 0.0)) < 2e-2
            and abs(pos.value("baseAmount", 0.0) - pgPos.value("baseAmount", 0.0)) < 2e-6
            and abs(pos.value("quoteAmount", 0.0) - pgPos.value("quoteAmount", 0.0)) < 2e-2
            and abs(pos.value("baseHeldAmount", 0.0) - pgPos.value("baseHeldAmount", 0.0)) < 2e-6
            and abs(pos.value("quoteHeldAmount", 0.0) - pgPos.value("quoteHeldAmount", 0.0)) < 2e-2
            and abs(pos.value("profitBase", 0.0) - pgPos.value("profitBase", 0.0)) < 2e-2
            and abs(pos.value("profitQuote", 0.0) - pgPos.value("profitQuote", 0.0)) < 2e-2
          ) return;
        }
        pgPos = pos;
        if (!eq) calcTargetBasePos();
        UI::uiSend(uiTXT::Position, pos, true);
      };
      static void calcWalletAfterOrder(json k) {
        if (pgPos.is_null()) return;
        double heldAmount = 0;
        double amount = (mSide)k.value("side", 0) == mSide::Ask
          ? pgPos.value("baseAmount", 0.0) + pgPos.value("baseHeldAmount", 0.0)
          : pgPos.value("quoteAmount", 0.0) + pgPos.value("quoteHeldAmount", 0.0);
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (it->second.value("side", 0) != k.value("side", 0)) return;
          double held = it->second.value("quantity", 0.0) * ((mSide)it->second.value("side", 0) == mSide::Bid ? it->second.value("price", 0.0) : 1);
          if (amount >= held) {
            amount -= held;
            heldAmount += held;
          }
        }
        calcWallet(mWallet(amount, heldAmount, (mSide)k["side"].get<int>() == mSide::Ask
            ? k["/pair/base"_json_pointer].get<int>()
            : k["/pair/quote"_json_pointer].get<int>()
        ));
      };
  };
}

#endif

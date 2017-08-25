#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  static json pgPos;
  static json pgSafety;
  static uv_timer_t pgStats_;
  static map<double, json> pgBuys;
  static map<double, json> pgSells;
  static map<int, json> pgWallet;
  static vector<json> pgProfit;
  static double pgTargetBasePos = 0;
  static string pgSideAPR = "";
  static string pgSideAPR_ = "!=";
  class PG {
    public:
      static void main(Local<Object> exports) {
        load();
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &pgStats_)) { cout << FN::uiT() << "Errrror: GW pgStats_ init timer failed." << endl; exit(1); }
          pgStats_.data = NULL;
          if (uv_timer_start(&pgStats_, [](uv_timer_t *handle) {
            if (mgfairV) calc();
            else cout << FN::uiT() << "Unable to calculate stats, missing fair value." << endl;
          }, 0, 1000)) { cout << FN::uiT() << "Errrror: GW pgStats_ start timer failed." << endl; exit(1); }
        }).detach();
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
          if (mgfairV) calcSafety();
        });
        EV::evOn("PositionBroker", [](json k) {
          calcTargetBasePos();
        });
        EV::evOn("OrderTradeBroker", [](json k) {
          tradeUp(k);
          if (mgfairV) calcSafety();
        });
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        UI::uiSnap(uiTXT::TradeSafetyValue, &onSnapSafety);
        UI::uiSnap(uiTXT::TargetBasePosition, &onSnapTargetBasePos);
        NODE_SET_METHOD(exports, "pgRepo", PG::_pgRepo);
        NODE_SET_METHOD(exports, "pgSideAPR", PG::_pgSideAPR);
        NODE_SET_METHOD(exports, "pgSafety", PG::_pgSafety);
        NODE_SET_METHOD(exports, "pgTargetBasePos", PG::_pgTargetBasePos);
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
      static void calc() {
        MG::calcStats();
        calcSafety();
      };
      static void calcSafety() {
        if (pgPos.is_null()) return;
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
          ? qpRepo["widthPongPercentage"].get<double>() * mgfairV / 100
          : qpRepo["widthPong"].get<double>();
        double buyPing = 0;
        double sellPong = 0;
        double buyPq = 0;
        double sellPq = 0;
        double _buyPq = 0;
        double _sellPq = 0;
        map<double, json> tradesBuy;
        map<double, json> tradesSell;
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          if ((mSide)(*it)["side"].get<int>() == mSide::Bid)
            tradesBuy[(*it)["price"].get<double>()] = *it;
          else tradesSell[(*it)["price"].get<double>()] = *it;
        if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingFair or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingAggressive) {
          for (map<double, json>::iterator it = tradesBuy.begin(); it != tradesBuy.end(); ++it) {
            if (buyPq<sellSize and (!mgfairV or (mgfairV>it->second["price"].get<double>() and mgfairV-widthPong<it->second["price"].get<double>())) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _buyPq = fmin(sellSize - buyPq, it->second["quantity"].get<double>());
              buyPing += it->second["price"].get<double>() * _buyPq;
              buyPq += _buyPq;
            }
            if (buyPq>=sellSize) break;
          }
          if (!buyPq) for (map<double, json>::reverse_iterator it = tradesBuy.rbegin(); it != tradesBuy.rend(); ++it) {
            if (buyPq<sellSize and (!mgfairV or mgfairV>it->second["price"].get<double>()) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _buyPq = fmin(sellSize - buyPq, it->second["quantity"].get<double>());
              buyPing += it->second["price"].get<double>() * _buyPq;
              buyPq += _buyPq;
            }
            if (buyPq>=sellSize) break;
          }
        } else if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingFair or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingAggressive)
          for (map<double, json>::iterator it = tradesBuy.begin(); it != tradesBuy.end(); ++it) {
            if (buyPq<sellSize and (!mgfairV or mgfairV>it->second["price"].get<double>()) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _buyPq = fmin(sellSize - buyPq, it->second["quantity"].get<double>());
              buyPing += it->second["price"].get<double>() * _buyPq;
              buyPq += _buyPq;
            }
            if (buyPq>=sellSize) break;
          }
        if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingFair or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::ShortPingAggressive) {
          for (map<double, json>::iterator it = tradesSell.begin(); it != tradesSell.end(); ++it) {
            if (sellPq<buySize and (!mgfairV or (mgfairV<it->second["price"].get<double>() and mgfairV+widthPong>it->second["price"].get<double>())) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _sellPq = fmin(buySize - sellPq, it->second["quantity"].get<double>());
              sellPong += it->second["price"].get<double>() * _sellPq;
              sellPq += _sellPq;
            }
            if (sellPq>=buySize) break;
          }
          if (!sellPq) for (map<double, json>::reverse_iterator it = tradesSell.rbegin(); it != tradesSell.rend(); ++it) {
            if (sellPq<buySize and (!mgfairV or mgfairV<it->second["price"].get<double>()) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _sellPq = fmin(buySize - sellPq, it->second["quantity"].get<double>());
              sellPong += it->second["price"].get<double>() * _sellPq;
              sellPq += _sellPq;
            }
            if (sellPq>=buySize) break;
          }
        } else if ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingFair or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingAggressive)
          for (map<double, json>::iterator it = tradesSell.begin(); it != tradesSell.end(); ++it) {
            if (sellPq<buySize and (!mgfairV or mgfairV<it->second["price"].get<double>()) and (((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Boomerang and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::HamelinRat and (mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::AK47) or it->second["Kqty"].get<double>()<it->second["quantity"].get<double>())) {
              _sellPq = fmin(buySize - sellPq, it->second["quantity"].get<double>());
              sellPong += it->second["price"].get<double>() * _sellPq;
              sellPq += _sellPq;
            }
            if (sellPq>=buySize) break;
          }
        if (buyPq) buyPing /= buyPq;
        if (sellPq) sellPong /= sellPq;
        cleanTrades();
        while (pgBuys.size() and pgSells.size()) {
          json buy = pgBuys.rbegin()->second;
          json sell = pgSells.begin()->second;
          if (sell["price"].get<double>() < buy["price"].get<double>()) break;
          double buyQty = buy["quantity"].get<double>();
          double sellQty = sell["quantity"].get<double>();
          buy["quantity"] = buyQty - sellQty;
          sell["quantity"] = sellQty - buyQty;
          if (buy["quantity"].get<double>() < gw->minSize)
            pgBuys.erase(--pgBuys.rbegin().base());
          if (sell["quantity"].get<double>() < gw->minSize)
            pgSells.erase(pgSells.begin());
        }
        double sumBuys = sumTrades(&pgBuys);
        double sumSells = sumTrades(&pgSells);
        json safety = {
          {"buy", sumBuys / buySize},
          {"sell", sumSells / sellSize},
          {"combined", (sumBuys + sumSells) / (buySize + sellSize)},
          {"buyPing", buyPing},
          {"sellPong", sellPong}
        };
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
      static void cleanTrades() {
        cleanTrades(&pgBuys);
        cleanTrades(&pgSells);
      };
      static json cleanTrades(map<double, json>* k) {
        unsigned long now = FN::T();
        for (map<double, json>::iterator it = k->begin(); it != k->end();)
          if (it->second["time"].get<unsigned long>() + qpRepo["tradeRateSeconds"].get<double>() * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      static double sumTrades(map<double, json>* k) {
        double sum = 0;
        for (map<double, json>::iterator it = k->begin(); it != k->end(); ++it)
          sum += it->second["quantity"].get<double>();
        return sum;
      };
      static json onSnapTargetBasePos(json z) {
        return {{
          {"tbp", pgTargetBasePos},
          {"sideAPR", pgSideAPR}
        }};
      };
      static void _pgTargetBasePos(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), pgTargetBasePos));
      };
      static void _pgSideAPR(const FunctionCallbackInfo<Value>& args) {
        pgSideAPR = FN::S8v(args[0]->ToString());
      };
      static void _pgRepo(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, pgPos.dump())).ToLocalChecked());
      };
      static void _pgSafety(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, pgSafety.dump())).ToLocalChecked());
      };
      static json onSnapPos(json z) {
        return { pgPos };
      };
      static json onSnapSafety(json z) {
        return { pgSafety };
      };
      static void posUp(json k) {
        if (!k.is_null()) pgWallet[k["currency"].get<int>()] = k;
        if (!mgfairV or pgWallet.find(gw->base) == pgWallet.end() or pgWallet.find(gw->quote) == pgWallet.end() or pgWallet[gw->base].is_null() or pgWallet[gw->quote].is_null()) return;
        double baseAmount = pgWallet[gw->base]["amount"].get<double>();
        double quoteAmount = pgWallet[gw->quote]["amount"].get<double>();
        double baseValue = baseAmount + quoteAmount / mgfairV + pgWallet[gw->base]["heldAmount"].get<double>() + pgWallet[gw->quote]["heldAmount"].get<double>() / mgfairV;
        double quoteValue = baseAmount * mgfairV + quoteAmount + pgWallet[gw->base]["heldAmount"].get<double>() * mgfairV + pgWallet[gw->quote]["heldAmount"].get<double>();
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
            ? qpRepo["targetBasePositionPercentage"].get<double>() * pgPos["value"].get<double>() / 100
            : qpRepo["targetBasePosition"].get<double>())
          : ((1 + mgTargetPos) / 2) * pgPos["value"].get<double>();
        if (pgTargetBasePos and abs(pgTargetBasePos - targetBasePosition) < 1e-4 and pgSideAPR_ == pgSideAPR) return;
        pgTargetBasePos = targetBasePosition;
        pgSideAPR_ = pgSideAPR;
        EV::evUp("TargetPosition");
        json k = {{"tbp", pgTargetBasePos}, {"sideAPR", pgSideAPR}};
        UI::uiSend(uiTXT::TargetBasePosition, k, true);
        DB::insert(uiTXT::TargetBasePosition, k);
        cout << FN::uiT() << "TBP = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << endl;
      };
  };
}

#endif

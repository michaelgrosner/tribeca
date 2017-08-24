#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  static json pgPos;
  static vector<json> pgDiff;
  static map<int, json> pgCur;
  static double pgTargetBasePos = 0;
  static string pgSideAPR = "";
  static string pgSideAPR_ = "!=";
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
        });
        EV::evOn("PositionBroker", [](json k) {
          calcTargetBasePos();
        });
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        UI::uiSnap(uiTXT::TargetBasePosition, &onSnapTargetBasePos);
        NODE_SET_METHOD(exports, "pgTargetBasePos", PG::_pgTargetBasePos);
        NODE_SET_METHOD(exports, "pgSideAPR", PG::_pgSideAPR);
        NODE_SET_METHOD(exports, "pgRepo", PG::_pgRepo);
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
        cout << FN::uiT() << "TBP " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << " loaded from DB." << endl;
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
      static json onSnapPos(json z) {
        return { pgPos };
      };
      static void posUp(json k) {
        if (!k.is_null()) pgCur[k["currency"].get<int>()] = k;
        if (!mgfairV or pgCur.find(gw->base) == pgCur.end() or pgCur.find(gw->quote) == pgCur.end() or pgCur[gw->base].is_null() or pgCur[gw->quote].is_null()) return;
        double baseAmount = pgCur[gw->base]["amount"].get<double>();
        double quoteAmount = pgCur[gw->quote]["amount"].get<double>();
        double baseValue = baseAmount + quoteAmount / mgfairV + pgCur[gw->base]["heldAmount"].get<double>() + pgCur[gw->quote]["heldAmount"].get<double>() / mgfairV;
        double quoteValue = baseAmount * mgfairV + quoteAmount + pgCur[gw->base]["heldAmount"].get<double>() * mgfairV + pgCur[gw->quote]["heldAmount"].get<double>();
        unsigned long now = FN::T();
        pgDiff.push_back({
          {"baseValue", baseValue},
          {"quoteValue", quoteValue},
          {"time", now }
        });
        for (vector<json>::iterator it = pgDiff.begin(); it != pgDiff.end();)
          if ((*it)["time"].get<unsigned long>() + (qpRepo["profitHourInterval"].get<double>() * 36e+5) > now) ++it;
          else it = pgDiff.erase(it);
        json pos = {
          {"baseAmount", baseAmount},
          {"quoteAmount", quoteAmount},
          {"baseHeldAmount", pgCur[gw->base]["heldAmount"].get<double>()},
          {"quoteHeldAmount", pgCur[gw->quote]["heldAmount"].get<double>()},
          {"value", baseValue},
          {"quoteValue", quoteValue},
          {"profitBase", ((baseValue - (*pgDiff.begin())["baseValue"].get<double>()) / baseValue) * 1e+2},
          {"profitQuote", ((quoteValue - (*pgDiff.begin())["quoteValue"].get<double>()) / quoteValue) * 1e+2},
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
        if (pgPos.is_null()) {
          cout << FN::uiT() << "Wallet Stats notice: missing market data." << endl;
          return;
        }
        double targetBasePosition = ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::Manual)
          ? (qpRepo["percentageValues"].get<bool>()
            ? qpRepo["targetBasePositionPercentage"].get<double>() * pgPos["value"].get<double>() / 100
            : qpRepo["targetBasePosition"].get<double>())
          : ((1 + mgTargetPos) / 2) * pgPos["value"].get<double>();
        if (pgTargetBasePos and abs(pgTargetBasePos - targetBasePosition) < 1e-4 and pgSideAPR_ == pgSideAPR) return;
        pgTargetBasePos = targetBasePosition;
        pgSideAPR_ = pgSideAPR;
        EV::evUp("TargetPosition");
        UI::uiSend(uiTXT::TargetBasePosition, {{"tbp", pgTargetBasePos},{"sideAPR", pgSideAPR}}, true);
        DB::insert(uiTXT::TargetBasePosition, {{"tbp", pgTargetBasePos},{"sideAPR", pgSideAPR}});
        cout << FN::uiT() << "TBP = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << endl;
      };
  };
}

#endif

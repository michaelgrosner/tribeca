#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  static json pgPos;
  static vector<json> pgDiff;
  static map<int, json> pgCur;
  class PG {
    public:
      static void main(Local<Object> exports) {
        EV::evOn("PositionGateway", [](json k) {
          posUp(k);
        });
        EV::evOn("OrderUpdateBroker", [](json k) {
          orderUp(k);
        });
        EV::evOn("FairValue", [](json k) {
          posUp({});
        });
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        NODE_SET_METHOD(exports, "pgRepo", PG::_pgRepo);
      };
    private:
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
        if (!eq) EV::evUp("PositionBroker");
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
  };
}

#endif

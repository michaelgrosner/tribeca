#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  static json pgWall;
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
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, pgWall.dump())).ToLocalChecked());
      };
      static json onSnapPos(json z) {
        return { pgWall };
      };
      static void posUp(json k) {
        if (!k.is_null()) pgCur[k["currency"]] = k;
        if (!mGWfairV or pgCur.find(gw->base) == pgCur.end() or pgCur.find(gw->quote) == pgCur.end() or pgCur[gw->base].is_null() or pgCur[gw->quote].is_null()) return;
        double baseAmount = pgCur[gw->base]["amount"].get<double>();
        double quoteAmount = pgCur[gw->quote]["amount"].get<double>();
        double baseValue = baseAmount + quoteAmount / mGWfairV + pgCur[gw->base]["heldAmount"].get<double>() + pgCur[gw->quote]["heldAmount"].get<double>() / mGWfairV;
        double quoteValue = baseAmount * mGWfairV + quoteAmount + pgCur[gw->base]["heldAmount"].get<double>() * mGWfairV + pgCur[gw->quote]["heldAmount"].get<double>();
        unsigned long now = FN::T();
        pgDiff.push_back({
          {"baseValue", baseValue},
          {"quoteValue", quoteValue},
          {"time", now }
        });
        for (vector<json>::iterator it = pgDiff.begin(); it != pgDiff.end();)
          if ((*it)["time"].get<unsigned long>() + (qpRepo["profitHourInterval"].get<double>() * 36e+5) > now) ++it;
          else it = pgDiff.erase(it);
        double profitBase = ((baseValue - (*pgDiff.begin())["baseValue"].get<double>()) / baseValue) * 1e+2;
        double profitQuote = ((quoteValue - (*pgDiff.begin())["quoteValue"].get<double>()) / quoteValue) * 1e+2;
        json pos = {
          {"baseAmount", baseAmount},
          {"quoteAmount", quoteAmount},
          {"baseHeldAmount", pgCur[gw->base]["heldAmount"].get<double>()},
          {"quoteHeldAmount", pgCur[gw->quote]["heldAmount"].get<double>()},
          {"value", baseValue},
          {"quoteValue", quoteValue},
          {"profitBase", profitBase},
          {"profitQuote", profitQuote},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"exchange", (int)gw->exchange}
        };
        bool eq = true;
        if (!pgWall.is_null()) {
          eq = abs(pos["value"].get<double>() - pgWall["value"].get<double>()) < 2e-6;
          if(eq &&
            abs(pos["quoteValue"].get<double>() - pgWall["quoteValue"].get<double>()) < 2e-2 &&
            abs(pos["baseAmount"].get<double>() - pgWall["baseAmount"].get<double>()) < 2e-6 &&
            abs(pos["quoteAmount"].get<double>() - pgWall["quoteAmount"].get<double>()) < 2e-2 &&
            abs(pos["baseHeldAmount"].get<double>() - pgWall["baseHeldAmount"].get<double>()) < 2e-6 &&
            abs(pos["quoteHeldAmount"].get<double>() - pgWall["quoteHeldAmount"].get<double>()) < 2e-2 &&
            abs(pos["profitBase"].get<double>() - pgWall["profitBase"].get<double>()) < 2e-2 &&
            abs(pos["profitQuote"].get<double>() - pgWall["profitQuote"].get<double>()) < 2e-2
          ) return;
        }
        pgWall = pos;
        if (!eq) EV::evUp("PositionBroker");
        UI::uiSend(uiTXT::Position, pos, true);
      };
      static void orderUp(json k) {
        if (pgWall.is_null()) return;
        double heldAmount = 0;
        double amount = (mSide)k["side"].get<int>() == mSide::Ask
          ? pgWall["baseAmount"].get<double>() + pgWall["baseHeldAmount"].get<double>()
          : pgWall["quoteAmount"].get<double>() + pgWall["quoteHeldAmount"].get<double>();
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

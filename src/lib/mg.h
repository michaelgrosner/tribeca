#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG {
    public:
      static void main(Local<Object> exports) {
        EV::evOn("MarketTradeGateway", [](Local<Object> o) {
          mGWmt t(
            gw->exchange,
            gw->base,
            gw->quote,
            o->Get(FN::v8S("price"))->NumberValue(),
            o->Get(FN::v8S("size"))->NumberValue(),
            FN::T(),
            (mSide)o->Get(FN::v8S("make_side"))->NumberValue()
          );
          mGWmt_.push_back(t);
          if (mGWmt_.size()>69) mGWmt_.erase(mGWmt_.begin());
          EV::evUp("MarketTrade");
          UI::uiSend(uiTXT::MarketTrade, v8mGWmt(t));
        });
        Isolate* isolate = exports->GetIsolate();
        mGWmkt.Reset(isolate, Object::New(isolate));
        EV::evOn("MarketDataGateway", [](Local<Object> o) {
          v8mGWmkt(o);
        });
        EV::evOn("GatewayMarketConnect", [](Local<Object> c) {
          if ((mConnectivityStatus)c->NumberValue() == mConnectivityStatus::Disconnected)
            v8mGWmkt();
        });
        UI::uiSnap(uiTXT::MarketData, &onSnapBook);
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
      };
    private:
      static json onSnapTrade(Local<Value> z) {
        json k;
        JSON Json;
        Isolate* isolate = Isolate::GetCurrent();
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(v8mGWmt(mGWmt_[i]));
        return k;
      };
      static json onSnapBook(Local<Value> z) {
        json k;
        JSON Json;
        Isolate* isolate = Isolate::GetCurrent();
        k.push_back(json::parse(FN::S8v(Json.Stringify(isolate->GetCurrentContext(), Local<Object>::New(isolate, mGWmkt)).ToLocalChecked())));
        return k;
      };
      static void v8mGWmkt() {
        Isolate* isolate = Isolate::GetCurrent();
        v8mGWmkt(Object::New(isolate));
      }
      static void v8mGWmkt(Local<Object> o) {
        Isolate* isolate = Isolate::GetCurrent();
        EV::evUp("MarketDataBroker", o);
        JSON Json;
        UI::uiSend(uiTXT::MarketData, json::parse(FN::S8v(Json.Stringify(isolate->GetCurrentContext(), o).ToLocalChecked())), true);
        mGWmkt.Reset(isolate, o);
      };
      static json v8mGWmt(mGWmt t) {
        json o = {
          {"exchange", (int)t.exchange},
          {"pair", {{"base", t.base}, {"quote", t.quote}}},
          {"price", t.price},
          {"size", t.size},
          {"time", t.time},
          {"make_size", (int)t.make_side}
        };
        return o;
      };
  };
}

#endif

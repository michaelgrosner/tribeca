#ifndef K_MD_H_
#define K_MD_H_

namespace K {
  struct mGWmt {
    mExchange exchange;
    int base;
    int quote;
    double price;
    double size;
    unsigned long time;
    mSide make_side;
    mGWmt(mExchange e_, int b_, int q_, double p_, double s_, double t_, mSide m_):
      exchange(e_), base(b_), quote(q_), price(p_), size(s_), time(t_), make_side(m_) {}
  };
  vector<mGWmt> mGWmt_;
  Persistent<Object> mGWmkt;
  class MD {
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
          UI::uiSend(Isolate::GetCurrent(), uiTXT::MarketTrade, v8mGWmt(t));
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
      static Local<Value> onSnapTrade(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        for (unsigned i=0; i<mGWmt_.size(); ++i) k->Set(i, v8mGWmt(mGWmt_[i]));
        return k;
      };
      static Local<Value> onSnapBook(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Local<Object>::New(isolate, mGWmkt));
        return k;
      };
      static void v8mGWmkt() {
        Isolate* isolate = Isolate::GetCurrent();
        v8mGWmkt(Object::New(isolate));
      }
      static void v8mGWmkt(Local<Object> o) {
        Isolate* isolate = Isolate::GetCurrent();
        EV::evUp("MarketDataBroker", o);
        UI::uiSend(isolate, uiTXT::MarketData, o, true);
        mGWmkt.Reset(isolate, o);
      };
      static Local<Object> v8mGWmt(mGWmt t) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("exchange"), Number::New(isolate, (double)t.exchange));
        Local<Object> k = Object::New(isolate);
        k->Set(FN::v8S("base"), Number::New(isolate, (double)t.base));
        k->Set(FN::v8S("quote"), Number::New(isolate, (double)t.quote));
        o->Set(FN::v8S("pair"), k);
        o->Set(FN::v8S("price"), Number::New(isolate, t.price));
        o->Set(FN::v8S("size"), Number::New(isolate, t.size));
        o->Set(FN::v8S("time"), Number::New(isolate, t.time));
        o->Set(FN::v8S("make_side"), Number::New(isolate, (double)t.make_side));
        return o;
      };
  };
}

#endif

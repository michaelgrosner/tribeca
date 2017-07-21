#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  string cFname;
  Persistent<Object> cfRepo;
  class CF {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = Isolate::GetCurrent();
        string k = string(getenv("KCONFIG") != NULL ? getenv("KCONFIG") : "K.json");
        cFname = string("etc/").append(k);
        if (access(cFname.data(), F_OK) != -1) {
          ifstream file(cFname);
          string txt((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
          JSON Json;
          cfRepo.Reset(isolate, Json.Parse(isolate->GetCurrentContext(), FN::v8S(txt.data())).ToLocalChecked()->ToObject());
          cout << FN::uiT() << "CF settings loaded from " << k << " OK." << endl;
        } else {
          cfRepo.Reset(isolate, Object::New(isolate));
          cout << FN::uiT() << "Warrrrning: CF settings not loaded because the config file was not found, reading ENVIRONMENT vars instead." << endl;
        }
        NODE_SET_METHOD(exports, "cfString", CF::_cfString);
        NODE_SET_METHOD(exports, "cfmExchange", CF::_cfmExchange);
        NODE_SET_METHOD(exports, "cfmCurrencyPair", CF::_cfmCurrencyPair);
      };
      static string cfString(string k, bool r = true) {
        Isolate* isolate = Isolate::GetCurrent();
        if (getenv(k.data()) != NULL)
          return string(getenv(k.data()));
        Local<Object> o = Local<Object>::New(isolate, cfRepo);
        MaybeLocal<Array> maybe_props = o->GetOwnPropertyNames(Context::New(isolate));
        Local<Array> props = maybe_props.ToLocalChecked();
        if (!maybe_props.IsEmpty())
          for(uint32_t i=0; i < props->Length(); i++) if (k == string(*String::Utf8Value(props->Get(i)->ToString())))
            return string(*String::Utf8Value(o->Get(props->Get(i)->ToObject())->ToString()));
        if (r) { cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" configuration." << endl; exit(1); }
        return "";
      };
      static int cfBase() {
        string k_ = cfString("TradedPair");
        string k = k_.substr(0, k_.find("/"));
        if (k == k_) { cout << FN::uiT() << "Errrror: Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        for (unsigned i=0; i<mCurrency.size(); ++i)
          if (mCurrency[i] == k) return i;
        cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" currency." << endl; exit(1);
      };
      static int cfQuote() {
        string k_ = cfString("TradedPair");
        string k = k_.substr(k_.find("/")+1);
        if (k == k_) { cout << FN::uiT() << "Errrror: Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        for (unsigned i=0; i<mCurrency.size(); ++i)
          if (mCurrency[i] == k) return i;
        cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" currency." << endl; exit(1);
      };
      static mExchange cfExchange() {
        string k = cfString("EXCHANGE");
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        if (k == "coinbase") return mExchange::Coinbase;
        else if (k == "okcoin") return mExchange::OkCoin;
        else if (k == "bitfinex") return mExchange::Bitfinex;
        else if (k == "poloniex") return mExchange::Poloniex;
        else if (k == "korbit") return mExchange::Korbit;
        else if (k == "hitbtc") return mExchange::HitBtc;
        else if (k ==  "null") return mExchange::Null;
        cout << FN::uiT() << "Errrror: Invalid configuration value \"" << k << "\" as EXCHANGE." << endl;
        exit(1);
      };
    private:
      static void _cfString(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(FN::v8S(cfString(FN::S8v(args[0]->ToString()))));
      };
      static void _cfmExchange(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, (double)cfExchange()));
      };
      static void _cfmCurrencyPair(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("base"), Number::New(isolate, (double)cfBase()));
        o->Set(FN::v8S("quote"), Number::New(isolate, (double)cfQuote()));
        args.GetReturnValue().Set(o);
      };
  };
}

#endif
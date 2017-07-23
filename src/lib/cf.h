#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  string cFname;
  json cfRepo;
  json pkRepo;
  class CF {
    public:
      static void main(Local<Object> exports) {
        if (access("package.json", F_OK) != -1) {
          ifstream file_("package.json");
          pkRepo = json::parse(string((istreambuf_iterator<char>(file_)), istreambuf_iterator<char>()));
        } else { cout << FN::uiT() << "Errrror: CF package.json not found." << endl; exit(1); }
        string k = string(getenv("KCONFIG") != NULL ? getenv("KCONFIG") : "K.json");
        cFname = string("etc/").append(k);
        if (access(cFname.data(), F_OK) != -1) {
          ifstream file(cFname);
          cfRepo = json::parse(string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>()));
          cout << FN::uiT() << "CF settings loaded from " << k << " OK." << endl;
        } else cout << FN::uiT() << "Warrrrning: CF settings not loaded because the config file was not found, reading ENVIRONMENT vars instead." << endl;
        NODE_SET_METHOD(exports, "cfString", CF::_cfString);
        NODE_SET_METHOD(exports, "cfmExchange", CF::_cfmExchange);
        NODE_SET_METHOD(exports, "cfmCurrencyPair", CF::_cfmCurrencyPair);
      };
      static string cfString(string k, bool r = true) {
        if (getenv(k.data()) != NULL) return string(getenv(k.data()));
        if (cfRepo.find(k) == cfRepo.end()) {
          if (r) {
            cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" configuration. See https://github.com/ctubio/Krypto-trading-bot/blob/master/etc/K.json.dist for latest example." << endl;
            exit(1);
          } else return "";
        }
        return (cfRepo[k].is_string()) ? cfRepo[k].get<string>() : to_string(cfRepo[k].get<double>());
      };
      static string cfPKString(string k) {
        if (pkRepo.find(k) == pkRepo.end()) {
          cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" package configuration." << endl;
          exit(1);
        }
        return pkRepo[k].get<string>();
      };
      static int cfBase() {
        string k_ = cfString("TradedPair");
        string k = k_.substr(0, k_.find("/"));
        if (k == k_) { cout << FN::uiT() << "Errrror: Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        return FN::S2mC(k);
      };
      static int cfQuote() {
        string k_ = cfString("TradedPair");
        string k = k_.substr(k_.find("/")+1);
        if (k == k_) { cout << FN::uiT() << "Errrror: Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        return FN::S2mC(k);
      };
      static mExchange cfExchange() {
        string k = FN::S2l(cfString("EXCHANGE"));
        if (k == "coinbase") return mExchange::Coinbase;
        else if (k == "okcoin") return mExchange::OkCoin;
        else if (k == "bitfinex") return mExchange::Bitfinex;
        else if (k == "poloniex") return mExchange::Poloniex;
        else if (k == "korbit") return mExchange::Korbit;
        else if (k == "hitbtc") return mExchange::HitBtc;
        else if (k == "null") return mExchange::Null;
        cout << FN::uiT() << "Errrror: Invalid configuration value \"" << k << "\" as EXCHANGE. See https://github.com/ctubio/Krypto-trading-bot/tree/master/etc#configuration-options for more information." << endl;
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
#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  static Gw* gw;
  static Gw* gW;
  static string dbFpath;
  static bool gwAutoStart = false;
  static Persistent<Object> qpRepo;
  static json pkRepo;
  static json cfRepo;
  static string cFname;
  class CF {
    public:
      static void internal(Local<Object> exports) {
        if (access("package.json", F_OK) != -1) {
          ifstream file_("package.json");
          pkRepo = json::parse(string((istreambuf_iterator<char>(file_)), istreambuf_iterator<char>()));
        } else { cout << FN::uiT() << "Errrror: CF package.json not found." << endl; exit(1); }
        string k = string(getenv("KCONFIG") != NULL ? getenv("KCONFIG") : "K");
        cFname = string("etc/").append(k).append(".json");
        string cfname = string("etc/").append(k).append(".png");
        if (access(cFname.data(), F_OK) != -1) {
          ifstream file(cFname);
          cfRepo = json::parse(string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>()));
          cout << FN::uiT() << "CF settings loaded from JSON file " << k << " OK." << endl;
        } else if (access(cfname.data(), F_OK) != -1) {
          cFname = cfname;
          png_structp png_ptr;
          png_infop info_ptr;
          unsigned char sig[8];
          FILE *fp;
          if(!(fp = fopen(cFname.data(), "rb"))) { cout << FN::uiT() << "Errrror: Could not find and open file " << k << "." << endl; }
          else {
            fread(sig, 1, 8, fp);
            if(!png_check_sig(sig, 8)) { cout << FN::uiT() << "Errrror: Not a PNG file." << endl; }
            else {
              png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
              info_ptr = png_create_info_struct(png_ptr);
              if(!png_ptr) { cout << FN::uiT() << "Errrror: Could not allocate memory." << endl; }
              else if (setjmp(png_jmpbuf(png_ptr))) { cout << FN::uiT() << "Errrror: PNG error." << endl; }
              else {
                png_init_io(png_ptr, fp);
                png_set_sig_bytes(png_ptr, 8);
                png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
                png_textp text_ptr;
                int num_text;
                png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
                string conf = "";
                for(int i = 0; i < num_text; i++)
                  if(strcmp("K.conf", text_ptr[i].key) == 0)
                    conf = text_ptr[i].text;
                if (conf.length()) {
                  cfRepo = json::parse(conf);
                  cout << FN::uiT() << "CF settings loaded from PNG file " << k << " OK." << endl;
                } else cout << FN::uiT() << "CF no data found inside PNG file " << k << "." << endl;
              }
            }
          }
        }
        if (cfString("EXCHANGE", false) == "") cout << FN::uiT() << "Warrrrning: CF settings not loaded because the config file was not found, reading ENVIRONMENT vars instead." << endl;
        NODE_SET_METHOD(exports, "cfString", CF::_cfString);
        NODE_SET_METHOD(exports, "cfmCurrencyPair", CF::_cfmCurrencyPair);
      };
      static void external() {
        gw = Gw::E(cfExchange());
        gw->base = cfBase();
        gw->quote = cfQuote();
        gw->config();
        if (!gw->minTick) { cout << FN::uiT() << "Errrror: Unable to match TradedPair to " << cfString("EXCHANGE") << " symbol \"" << gw->symbol << "\"." << endl; exit(1); }
        else { cout << FN::uiT() << "GW " << cfString("EXCHANGE") << " allows client IP." << endl; }
        gwAutoStart = "auto" == cfString("BotIdentifier").substr(0,4);
        cout << FN::uiT() << "GW " << setprecision(8) << fixed << cfString("EXCHANGE") << ":" << endl << "- autoBot: " << (gwAutoStart ? "yes" : "no") << endl << "- pair: " << gw->symbol << endl << "- minTick: " << gw->minTick << endl << "- minSize: " << gw->minSize << endl << "- makeFee: " << gw->makeFee << endl << "- takeFee: " << gw->takeFee << endl;
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
      static void _cfmCurrencyPair(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("base"), Number::New(isolate, cfBase()));
        o->Set(FN::v8S("quote"), Number::New(isolate, cfQuote()));
        args.GetReturnValue().Set(o);
      };
  };
}

#endif
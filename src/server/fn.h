#ifndef K_FN_H_
#define K_FN_H_

#ifndef K_BUILD
#define K_BUILD "0"
#endif

#ifndef K_STAMP
#define K_STAMP "0"
#endif

namespace K {
  static char RBLACK[]  = "\033[0;30m";
  static char RRED[]    = "\033[0;31m";
  static char RGREEN[]  = "\033[0;32m";
  static char RYELLOW[] = "\033[0;33m";
  static char RBLUE[]   = "\033[0;34m";
  static char RPURPLE[] = "\033[0;35m";
  static char RCYAN[]   = "\033[0;36m";
  static char RWHITE[]  = "\033[0;37m";
  static char BBLACK[]  = "\033[1;30m";
  static char BRED[]    = "\033[1;31m";
  static char BGREEN[]  = "\033[1;32m";
  static char BYELLOW[] = "\033[1;33m";
  static char BBLUE[]   = "\033[1;34m";
  static char BPURPLE[] = "\033[1;35m";
  static char BCYAN[]   = "\033[1;36m";
  static char BWHITE[]  = "\033[1;37m";
  static int argColors = 0;
  static int argDebugOrders = 0;
  static int argDebugQuotes = 0;
  static int argHeadless = 0;
  static double argEwmaShort = 0;
  static double argEwmaMedium = 0;
  static double argEwmaLong = 0;
  static string argDatabase = "";
  class FN {
    public:
      static void main(int argc, char** argv) {
        cout << BGREEN << "K" << RGREEN << " build " << K_BUILD << " " << K_STAMP << "." << BRED << endl;
        int k;
        while (true) {
          int i = 0;
          static struct option args[] = {
            {"help",         no_argument,       0,               'h'},
            {"colors",       no_argument,       &argColors,        1},
            {"debug-orders", no_argument,       &argDebugOrders,   1},
            {"debug-quotes", no_argument,       &argDebugQuotes,   1},
            {"headless",     no_argument,       &argHeadless,      1},
            {"database",     required_argument, 0,               'd'},
            {"ewma-short",   required_argument, 0,               's'},
            {"ewma-medium",  required_argument, 0,               'm'},
            {"ewma-long",    required_argument, 0,               'l'},
            {"version",      no_argument,       0,               'v'},
            {0,              0,                 0,                 0}
          };
          k = getopt_long(argc, argv, "hvd:l:m:s:", args, &i);
          if (k == -1) break;
          switch (k) {
            case 0: break;
            case 'd': argDatabase = string(optarg); break;
            case 's': argEwmaShort = stod(optarg); break;
            case 'm': argEwmaMedium = stod(optarg); break;
            case 'l': argEwmaLong = stod(optarg); break;
            case 'h': cout
              << RGREEN << "This is free software: the quoting engine and UI are open source," << endl << "feel free to hack both as you need." << endl
              << RGREEN << "This is non-free software: the exchange integrations are licensed" << endl << "by and under the law of my grandma, feel free to crack all." << endl
              << BGREEN << " " << RGREEN << " questions: " << RYELLOW << "https://21.co/analpaper/" << endl
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << endl
              << BGREEN << " " << RGREEN << " downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << endl;
            case '?': cout
              << FN::uiT() << "Usage:" << BYELLOW << " ./K.sh [arguments]" << endl
              << FN::uiT() << "[arguments]:" << endl
              << FN::uiT() << RWHITE << "-h, --help               - show this help and quit." << endl
              << FN::uiT() << RWHITE << "    --colors             - print highlighted output." << endl
              << FN::uiT() << RWHITE << "    --debug-orders       - print detailed output about exchange messages." << endl
              << FN::uiT() << RWHITE << "    --debug-quotes       - print detailed output about quoting engine." << endl
              << FN::uiT() << RWHITE << "    --headless           - do not listen for UI connections." << endl
              << FN::uiT() << RWHITE << "-d, --database=PATH      - set alternative database filename," << endl
              << FN::uiT() << RWHITE << "                           default PATH is '/data/db/K.*.*.*.db'," << endl
              << FN::uiT() << RWHITE << "                           any path with a filename is valid," << endl
              << FN::uiT() << RWHITE << "                           or use ':memory:' (sqlite.org/inmemorydb.html)." << endl
              << FN::uiT() << RWHITE << "-s, --ewma-short=PRICE   - set initial ewma short value." << endl
              << FN::uiT() << RWHITE << "-m, --ewma-medium=PRICE  - set initial ewma medium value." << endl
              << FN::uiT() << RWHITE << "-l, --ewma-long=PRICE    - set initial ewma long value." << endl
              << FN::uiT() << RWHITE << "-v, --version            - show current build version and quit." << endl
              << BGREEN << " " << RGREEN << " more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << endl
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.domirc.net:6667/##tradingBot" << endl
              << BGREEN << " " << RGREEN << " home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << endl;
              exit(EXIT_SUCCESS);
              break;
            case 'v': exit(EXIT_SUCCESS);
            default: abort();
          }
        }
        if (optind < argc) {
          cout << FN::uiT() << "ARG" << RRED << " Warrrrning:" << BRED << " non-option ARGV-elements: ";
          while(optind < argc) cout << argv[optind++];
          cout << endl;
        }
        cout << RWHITE;
        if (!argColors) {
          RBLACK[0]  = 0; RRED[0]    = 0; RGREEN[0]  = 0; RYELLOW[0] = 0;
          RBLUE[0]   = 0; RPURPLE[0] = 0; RCYAN[0]   = 0; RWHITE[0]  = 0;
          BBLACK[0]  = 0; BRED[0]    = 0; BGREEN[0]  = 0; BYELLOW[0] = 0;
          BBLUE[0]   = 0; BPURPLE[0] = 0; BCYAN[0]   = 0; BWHITE[0]  = 0;
        }
      };
      static string S2l(string k) { transform(k.begin(), k.end(), k.begin(), ::tolower); return k; };
      static string S2u(string k) { transform(k.begin(), k.end(), k.begin(), ::toupper); return k; };
      static double roundNearest(double value, double minTick) { return round(value / minTick) * minTick; };
      static double roundUp(double value, double minTick) { return ceil(value / minTick) * minTick; };
      static double roundDown(double value, double minTick) { return floor(value / minTick) * minTick; };
      static double roundSide(double oP, double minTick, mSide oS) {
        if (oS == mSide::Bid) return roundDown(oP, minTick);
        else if (oS == mSide::Ask) return roundUp(oP, minTick);
        else return roundNearest(oP, minTick);
      };
      static unsigned long T() { return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count(); };
      static string uiT() {
        typedef chrono::duration<int, ratio_multiply<chrono::hours::period, ratio<24>>::type> fnT;
        chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
        auto t = now.time_since_epoch();
        fnT days = chrono::duration_cast<fnT>(t);
        t -= days;
        auto hours = chrono::duration_cast<chrono::hours>(t);
        t -= hours;
        auto minutes = chrono::duration_cast<chrono::minutes>(t);
        t -= minutes;
        auto seconds = chrono::duration_cast<chrono::seconds>(t);
        t -= seconds;
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(t);
        t -= milliseconds;
        auto microseconds = chrono::duration_cast<chrono::microseconds>(t);
        stringstream T;
        T << BGREEN << setfill('0')
          << setw(2) << hours.count() << ":"
          << setw(2) << minutes.count() << ":"
          << setw(2) << seconds.count() << RGREEN << "."
          << setw(3) << milliseconds.count()
          << setw(3) << microseconds.count() << BWHITE << " ";
        return T.str();
      };
      static string oHex(string k) {
       int len = k.length();
        string k_;
        for(int i=0; i< len; i+=2) {
          string byte = k.substr(i,2);
          char chr = (char) (int)strtol(byte.data(), NULL, 16);
          k_.push_back(chr);
        }
        return k_;
      };
      static string oMd5(string k) {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[16*2+1];
        for(int i = 0; i < 16; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return S2u(k_);
      };
      static string oSha512(string k) {
        unsigned char digest[SHA512_DIGEST_LENGTH];
        SHA512((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[SHA512_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oHmac256(string p, string s) {
        unsigned char* digest;
        digest = HMAC(EVP_sha256(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA256_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return oHex(k_);
      };
      static string oHmac512(string p, string s) {
        unsigned char* digest;
        digest = HMAC(EVP_sha512(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA512_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oHmac384(string p, string s) {
        unsigned char* digest;
        digest = HMAC(EVP_sha384(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA384_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA384_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static json wJet(string k) {
        return json::parse(wGet(k));
      };
      static string wGet(string k) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wGet failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p) {
        return json::parse(wGet(k, p));
      };
      static string wGet(string k, string p) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string t, bool auth) {
        return json::parse(wGet(k, t, auth));
      };
      static string wGet(string k, string t, bool auth) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          if (t != "") h_ = curl_slist_append(h_, string("Authorization: Bearer ").append(t).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p, string s) {
        return json::parse(wGet(k, p, s));
      };
      static string wGet(string k, string p, string s) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          h_ = curl_slist_append(h_, string("X-Signature: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wGet failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p, string s, bool post) {
        return json::parse(wGet(k, p, s, post));
      };
      static string wGet(string k, string p, string s, bool post) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, string("X-Signature: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p, string a, string s) {
        return json::parse(wGet(k, p, a, s));
      };
      static string wGet(string k, string p, string a, string s) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          h_ = curl_slist_append(h_, string("Key: ").append(a).data());
          h_ = curl_slist_append(h_, string("Sign: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p, string a, string s, bool post) {
        return json::parse(wGet(k, p, a, s, post));
      };
      static string wGet(string k, string p, string a, string s, bool post) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, string("X-BFX-APIKEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("X-BFX-PAYLOAD: ").append(p).data());
          h_ = curl_slist_append(h_, string("X-BFX-SIGNATURE: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << " at " << k << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string p, string a, string s, bool post, bool auth) {
        return json::parse(wGet(k, p, a, s, post, auth));
      };
      static string wGet(string k, string p, string t, string s, bool post, bool auth) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          if (t != "") h_ = curl_slist_append(h_, string("Authorization: Bearer ").append(t).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string t, string a, string s, string p) {
        return json::parse(wGet(k, t, a, s, p));
      };
      static string wGet(string k, string t, string a, string s, string p) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          h_ = curl_slist_append(h_, string("CB-ACCESS-KEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-SIGN: ").append(s).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-TIMESTAMP: ").append(t).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-PASSPHRASE: ").append(p).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wGet failed " << curl_easy_strerror(r) << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, string t, string a, string s, string p, bool d) {
        return json::parse(wGet(k, t, a, s, p, d));
      };
      static string wGet(string k, string t, string a, string s, string p, bool d) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          h_ = curl_slist_append(h_, string("CB-ACCESS-KEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-SIGN: ").append(s).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-TIMESTAMP: ").append(t).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-PASSPHRASE: ").append(p).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) cout << "CURL wGed failed " << curl_easy_strerror(r) << endl;
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static size_t wcb(void *buf, size_t size, size_t nmemb, void *up) {
        ((string*)up)->append((char*)buf, size * nmemb);
        return size * nmemb;
      };
      static string toP(double num, int n) {
        if(num == 0) return "0";
        double d = ceil(log10(num < 0 ? -num : num));
        int power = n - (int)d;
        double magnitude = pow(10., power);
        long shifted = ::round(num*magnitude);
        ostringstream oss;
        oss << shifted / magnitude;
        return oss.str();
      };
      static int procSelfStatus(char* line){
        int i = strlen(line);
        const char* p = line;
        while (*p <'0' || *p > '9') p++;
        line[i-3] = '\0';
        i = atoi(p);
        return i;
      };
      static int memory() {
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];
        while (fgets(line, 128, file) != NULL)
          if (strncmp(line, "VmRSS:", 6) == 0) {
            result = procSelfStatus(line);
            break;
          }
        fclose(file);
        return result * 1e+3;
      };
      static string output(string cmd) {
        string data;
        FILE * stream;
        const int max_buffer = 256;
        char buffer[max_buffer];
        cmd.append(" 2>&1");
        stream = popen(cmd.c_str(), "r");
        if (stream) {
          while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
          pclose(stream);
        }
        return data;
      };
  };
}

#endif

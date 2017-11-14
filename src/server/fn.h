#ifndef K_FN_H_
#define K_FN_H_

#ifndef K_BUILD
#define K_BUILD "0"
#endif

#ifndef K_STAMP
#define K_STAMP "0"
#endif

namespace K {
  class FN {
    public:
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
        chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
        auto t = now.time_since_epoch();
        auto days = chrono::duration_cast<chrono::duration<int, ratio_multiply<chrono::hours::period, ratio<24>>::type>>(t);
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
        stringstream T, T_;
        T << setfill('0') << setw(2) << hours.count() << ":" << setw(2) << minutes.count() << ":" << setw(2) << seconds.count();
        T_ << "." << setfill('0') << setw(3) << milliseconds.count() << setw(3) << microseconds.count();
        if (!wInit) return string(BGREEN) + T.str() + RGREEN + T_.str() + BWHITE + " ";
        wattron(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, A_BOLD);
        wprintw(wLog, T.str().data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, T_.str().data());
        wattroff(wLog, COLOR_PAIR(COLOR_GREEN));
        wprintw(wLog, " ");
        return "";
      };
      static string int64Id() {
        static random_device rd;
        static mt19937_64 gen(rd());
        uniform_int_distribution<unsigned long long> dis;
        return to_string(dis(gen)).substr(0,8);
      };
      static string charId() {
        char s[16];
        for (int i = 0; i < 16; ++i) s[i] = kB64Alphabet[stol(int64Id()) % (sizeof(kB64Alphabet) - 3)];
        return string(s, 16);
      };
      static string uuidId() {
        static const char alphanum[] = "0123456789"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "abcdefghijklmnopqrstuvwxyz";
        string uuid = string(36,' ');
        unsigned long rnd = stol(int64Id());
        unsigned long rnd_ = stol(int64Id());
        uuid[8] = '-';
        uuid[13] = '-';
        uuid[18] = '-';
        uuid[23] = '-';
        uuid[14] = '4';
        for(int i=0;i<36;i++)
          if (i != 8 && i != 13 && i != 18 && i != 14 && i != 23) {
            if (rnd <= 0x02) { rnd = 0x2000000 + (rnd_ * 0x1000000) | 0; }
            rnd >>= 4;
            uuid[i] = alphanum[(i == 19) ? ((rnd & 0xf) & 0x3) | 0x8 : rnd & 0xf];
          }
        return S2l(uuid);
      };
      static string oHex(string k) {
       int len = k.length();
        string k_;
        for(int i=0; i< len; i+=2) {
          string byte = k.substr(i,2);
          char chr = (char)(int)strtol(byte.data(), NULL, 16);
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
      static string oHmac256(string p, string s, bool hex) {
        unsigned char* digest;
        digest = HMAC(EVP_sha256(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA256_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return hex ? oHex(k_) : k_;
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
      static void stunnel() {
        system("test -n \"`/bin/pidof stunnel`\" && kill -9 `/bin/pidof stunnel`");
        system("stunnel etc/K-stunnel.conf");
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
          curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) FN::logWar("JSON", string("wGet failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (!k_.length() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json wJet(string k, bool a, string p) {
        return json::parse(wGet(k, a, p));
      };
      static string wGet(string k, bool a, string p) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          if (a) curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_USERPWD, p.data());
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) FN::logWar("JSON", string("wGet failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wPost failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wGet failed ") + curl_easy_strerror(r));
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
          if(r != CURLE_OK) FN::logWar("JSON", string("wGet failed ") + curl_easy_strerror(r));
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
      static string readlink(const char* pathname) {
        string buffer(64, '\0');
        ssize_t len;
        while((len = ::readlink(pathname, &buffer[0], buffer.size())) == static_cast<ssize_t>(buffer.size()))
          buffer.resize(buffer.size() * 2);
        if (len == -1) FN::logWar("FN", "readlink failed");
        buffer.resize(len);
        return buffer;
      };
      static void logWar(string k, string s) {
        logErr(k, s, " Warrrrning: ");
      };
      static void logExit(string k, string s, int code, bool ev = true) {
        FN::screen_quit();
        logErr(k, s);
        if (ev) (*evExit)(code);
        else exit(code);
      };
      static void logErr(string k, string s, string m = " Errrror: ") {
        if (!wInit) {
          cout << uiT() << k << RRED << m << BRED << s << ".\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_RED));
        wprintw(wLog, m.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, s.data());
        wattroff(wLog, COLOR_PAIR(COLOR_RED));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      static void logDB(string k) {
        if (!wInit) {
          cout << uiT() << "DB " << RYELLOW << k << RWHITE << " loaded OK.\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "DB ");
        wattroff(wLog, A_BOLD);
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " loaded OK.\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      static void logUI(string k, int p) {
        if (!wInit) {
          cout << uiT() << "UI" << RWHITE << " ready over " << RYELLOW << k << RWHITE << " on external port " << RYELLOW << to_string(p) << RWHITE << ".\n";
          return;
        }
        wMutex.lock();
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "UI");
        wattroff(wLog, A_BOLD);
        wprintw(wLog, " ready over ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " on external port ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, to_string(p).data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wMutex.unlock();
        FN::screen_refresh(k, p);
      };
      static void logUIsess(int k, string s) {
        if (!wInit) {
          cout << uiT() << "UI " << RYELLOW << to_string(k) << RWHITE << " currently connected, last connection was from " << RYELLOW << s << RWHITE << ".\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "UI ");
        wattroff(wLog, A_BOLD);
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, to_string(k).data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " currently connected, last connection was from ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, s.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      static void logVer(string k, int c) {
        if (!wInit) {
          cout << BGREEN << "K" << RGREEN << string(" version ").append(c == -1 ? "unknown (zip install).\n" : (!c ? "0day.\n" : string("-").append(to_string(c)).append("commit").append(c > 1?"s..\n":"..\n"))) << RYELLOW << (c ? k : "") << RWHITE;
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        wattron(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "K");
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" version ").append(c == -1 ? "unknown (zip install).\n" : (!c ? "0day.\n" : string("-").append(to_string(c)).append("commit").append(c > 1?"s..\n":"..\n"))).data());
        wattroff(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        if (c) wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wrefresh(wLog);
      };
      static void log(mTrade k, string e) {
        if (!wInit) {
          cout << FN::uiT() << "GW " << (k.side == mSide::Bid ? RCYAN : RPURPLE) << e << " TRADE " << (k.side == mSide::Bid ? BCYAN : BPURPLE) << (k.side == mSide::Bid ? "BUY " : "SELL ") << k.quantity << " " << k.pair.base << " at price " << k.price << " " << k.pair.quote << " (value " << k.value << " " << k.pair.quote << ").\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, A_BOLD);
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, "GW ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(k.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        wprintw(wLog, string(e).append(" TRADE ").data());
        wattroff(wLog, A_BOLD);
        stringstream ss;
        ss << setprecision(8) << fixed << (k.side == mSide::Bid ? "BUY " : "SELL ") << k.quantity << " " << k.pair.base << " at price " << k.price << " " << k.pair.quote << " (value " << k.value << " " << k.pair.quote << ")";
        wprintw(wLog, ss.str().data());
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(k.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        wrefresh(wLog);
      };
      static void log(string k, string s, string v) {
        if (!wInit) {
          cout << uiT() << k << RWHITE << " " << s << " " << RYELLOW << v << RWHITE << ".\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" ").append(s).append(" ").data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, v.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      static void log(string k, string s) {
        if (!wInit) {
          cout << uiT() << k << RWHITE << " " << s << ".\n";
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        uiT();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" ").append(s).append(".\n").data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      static void log(string k, int c = COLOR_WHITE, bool b = false) {
        if (!wInit) {
          cout << RWHITE << k;
          return;
        }
        lock_guard<mutex> lock(wMutex);
        wmove(wLog, getmaxy(wLog)-1, 0);
        if (b) wattron(wLog, A_BOLD);
        wattron(wLog, COLOR_PAIR(c));
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(c));
        if (b) wattroff(wLog, A_BOLD);
        wrefresh(wLog);
      };
      static void screen_quit() {
        if (!wInit) return;
        lock_guard<mutex> lock(wMutex);
        wInit = false;
        beep();
        endwin();
      };
      static void screen(int argColors, string argExchange, string argCurrency) {
        if ((wBorder = initscr()) == NULL) {
          cout << "NCURSES" << RRED << " Errrror:" << BRED << " Unable to initialize ncurses, try to run in your terminal \"export TERM=xterm\", or use --naked argument." << '\n';
          exit(EXIT_SUCCESS);
        }
        if (argColors) start_color();
        use_default_colors();
        cbreak();
        noecho();
        keypad(wBorder, true);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        wLog = subwin(wBorder, getmaxy(wBorder)-4, getmaxx(wBorder)-2, 3, 2);
        scrollok(wLog, true);
        idlok(wLog, true);
        signal(SIGWINCH, screen_resize);
        thread([&]() {
          int ch;
          while ((ch = wgetch(wBorder)) != 'q') {
            switch (ch) {
              case ERR: continue;
              // case KEY_PPAGE: wscrl(wLog, -3); wrefresh(wLog); break;
              // case KEY_NPAGE: wscrl(wLog, 3); wrefresh(wLog); break;
              // case KEY_UP: wscrl(wLog, -1); wrefresh(wLog); break;
              // case KEY_DOWN: wscrl(wLog, 1); wrefresh(wLog); break;
            }
          }
          screen_quit();
          cout << FN::uiT() << "Excellent decision!" << '\n';
          (*evExit)(EXIT_SUCCESS);
        }).detach();
        wInit = true;
        screen_refresh("", 0, argExchange, argCurrency);
      };
      static void screen_refresh(string protocol = "", int argPort = 0, string argExchange = "", string argCurrency = "") {
        if (!wInit) return;
        static int p = 0, spin = 0, port = 0;
        static string prtcl = "?", exchange = "?", currency = "?";
        if (argPort) port = argPort;
        if (protocol.length()) prtcl = protocol;
        if (argExchange.length()) exchange = argExchange;
        if (argCurrency.length()) currency = argCurrency;
        multimap<double, mOrder> orderLines;
        ogMutex.lock();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (mORS::Working != it->second.orderStatus) continue;
          orderLines.insert(pair<double, mOrder>(it->second.price, it->second));
        }
        ogMutex.unlock();
        lock_guard<mutex> lock(wMutex);
        int l = p,
            y = getmaxy(wBorder),
            x = getmaxx(wBorder),
            k = y - orderLines.size() - 1,
            P = k;
        while (l<y) mvwhline(wBorder, l++, 1, ' ', x-1);
        if (k!=p) {
          if (k<p) wscrl(wLog, p-k);
          wresize(wLog, k-3, x-2);
          if (k>p) wscrl(wLog, p-k);
          wrefresh(wLog);
          p = k;
        }
        mvwvline(wBorder, 1, 1, ' ', y-1);
        mvwvline(wBorder, k-1, 1, ' ', y-1);
        for (map<double, mOrder>::reverse_iterator it = orderLines.rbegin(); it != orderLines.rend(); ++it) {
          wattron(wBorder, COLOR_PAIR(it->second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
          stringstream ss;
          ss << setprecision(8) << fixed << (it->second.side == mSide::Bid ? "BID" : "ASK") << " > " << it->second.orderId << ": " << it->second.quantity << " " << it->second.pair.base << " at price " << it->second.price << " " << it->second.pair.quote;
          mvwaddstr(wBorder, ++P, 1, ss.str().data());
          wattroff(wBorder, COLOR_PAIR(it->second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        }
        mvwaddch(wBorder, 0, 0, ACS_ULCORNER);
        mvwhline(wBorder, 0, 1, ACS_HLINE, max(80, x));
        mvwvline(wBorder, 1, 0, ACS_VLINE, k);
        mvwvline(wBorder, k, 0, ACS_LTEE, y);
        mvwaddch(wBorder, y, 0, ACS_BTEE);
        mvwaddch(wBorder, 0, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddstr(wBorder, 0, 13, (string("   ") + K_BUILD + " " + K_STAMP + " ").data());
        mvwaddch(wBorder, 0, 14, 'K' | A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, 0, 18+string(K_BUILD).length()+string(K_STAMP).length(), ACS_LTEE);
        mvwaddch(wBorder, 0, x-12, ACS_RTEE);
        mvwaddstr(wBorder, 0, x-11, " [ ]: Quit!");
        mvwaddch(wBorder, 0, x-9, 'q' | A_BOLD);
        mvwaddch(wBorder, 0, 7, ACS_TTEE);
        mvwaddch(wBorder, 1, 7, ACS_LLCORNER);
        mvwhline(wBorder, 1, 8, ACS_HLINE, 4);
        mvwaddch(wBorder, 1, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 1, 14, (exchange + " " + currency).data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, (port ? " UI on " + prtcl + " port " + to_string(port) : " headless").data());
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, k, 0, ACS_LTEE);
        mvwhline(wBorder, k, 1, ACS_HLINE, 3);
        mvwaddch(wBorder, k, 4, ACS_RTEE);
        mvwaddstr(wBorder, k, 5, "< (");
        wattron(wBorder, COLOR_PAIR(COLOR_YELLOW));
        waddstr(wBorder, to_string(orderLines.size()).data());
        wattroff(wBorder, COLOR_PAIR(COLOR_YELLOW));
        waddstr(wBorder, ") Open Orders..");
        mvwaddch(wBorder, y-1, 0, ACS_LLCORNER);
        mvwaddstr(wBorder, y-1, x-1, string("|/-\\").substr(++spin, 1).data());
        if (spin==3) { spin = -1; }
        move(k-1, 2);
        wrefresh(wBorder);
        wrefresh(wLog);
      };
      static void screen_resize(int sig) {
        if (!wInit) return;
        wMutex.lock();
        struct winsize ws;
        if (ioctl(0, TIOCGWINSZ, &ws) < 0 or (ws.ws_row == getmaxy(wBorder) and ws.ws_col == getmaxx(wBorder))) {
          wMutex.unlock();
          return;
        }
        if (ws.ws_row < 10) ws.ws_row = 10;
        if (ws.ws_col < 20) ws.ws_col = 20;
        wresize(wBorder, ws.ws_row, ws.ws_col);
        resizeterm(ws.ws_row, ws.ws_col);
        wMutex.unlock();
        screen_refresh();
        lock_guard<mutex> lock(wMutex);
        redrawwin(wLog);
        wrefresh(wLog);
      };
      static void close(uv_loop_t* loop) {
        uv_walk(loop, close_walk_cb, NULL);
        uv_run(loop, UV_RUN_DEFAULT);
      };
      static void close_walk_cb(uv_handle_t* handle, void* arg) {
        if (!uv_is_closing(handle))
          uv_close(handle, NULL);
      };
  };
  class B64 {
   public:
    static bool Encode(const std::string &in, std::string *out) {
      int i = 0, j = 0;
      size_t enc_len = 0;
      unsigned char a3[3];
      unsigned char a4[4];
      out->resize(EncodedLength(in));
      int input_len = in.size();
      std::string::const_iterator input = in.begin();
      while (input_len--) {
        a3[i++] = *(input++);
        if (i == 3) {
          a3_to_a4(a4, a3);
          for (i = 0; i < 4; i++) (*out)[enc_len++] = kB64Alphabet[a4[i]];
          i = 0;
        }
      }
      if (i) {
        for (j = i; j < 3; j++) a3[j] = '\0';
        a3_to_a4(a4, a3);
        for (j = 0; j < i + 1; j++) (*out)[enc_len++] = kB64Alphabet[a4[j]];
        while ((i++ < 3)) (*out)[enc_len++] = '=';
      }
      return (enc_len == out->size());
    }
    static bool Encode(const char *input, size_t input_length, char *out, size_t out_length) {
      int i = 0, j = 0;
      char *out_begin = out;
      unsigned char a3[3];
      unsigned char a4[4];
      size_t encoded_length = EncodedLength(input_length);
      if (out_length < encoded_length) return false;
      while (input_length--) {
        a3[i++] = *input++;
        if (i == 3) {
          a3_to_a4(a4, a3);
          for (i = 0; i < 4; i++) *out++ = kB64Alphabet[a4[i]];
          i = 0;
        }
      }
      if (i) {
        for (j = i; j < 3; j++) a3[j] = '\0';
        a3_to_a4(a4, a3);
        for (j = 0; j < i + 1; j++) *out++ = kB64Alphabet[a4[j]];
        while ((i++ < 3)) *out++ = '=';
      }
      return (out == (out_begin + encoded_length));
    }
    static bool Decode(const std::string &in, std::string *out) {
      int i = 0, j = 0;
      size_t dec_len = 0;
      unsigned char a3[3];
      unsigned char a4[4];
      int input_len = in.size();
      std::string::const_iterator input = in.begin();
      out->resize(DecodedLength(in));
      while (input_len--) {
        if (*input == '=') break;
        a4[i++] = *(input++);
        if (i == 4) {
          for (i = 0; i <4; i++) a4[i] = b64_lookup(a4[i]);
          a4_to_a3(a3,a4);
          for (i = 0; i < 3; i++) (*out)[dec_len++] = a3[i];
          i = 0;
        }
      }
      if (i) {
        for (j = i; j < 4; j++) a4[j] = '\0';
        for (j = 0; j < 4; j++) a4[j] = b64_lookup(a4[j]);
        a4_to_a3(a3,a4);
        for (j = 0; j < i - 1; j++) (*out)[dec_len++] = a3[j];
      }
      return (dec_len == out->size());
    }

    static bool Decode(const char *input, size_t input_length, char *out, size_t out_length) {
      int i = 0, j = 0;
      char *out_begin = out;
      unsigned char a3[3];
      unsigned char a4[4];
      size_t decoded_length = DecodedLength(input, input_length);
      if (out_length < decoded_length) return false;
      while (input_length--) {
        if (*input == '=') break;
        a4[i++] = *(input++);
        if (i == 4) {
          for (i = 0; i <4; i++) a4[i] = b64_lookup(a4[i]);
          a4_to_a3(a3,a4);
          for (i = 0; i < 3; i++) *out++ = a3[i];
          i = 0;
        }
      }
      if (i) {
        for (j = i; j < 4; j++) a4[j] = '\0';
        for (j = 0; j < 4; j++) a4[j] = b64_lookup(a4[j]);
        a4_to_a3(a3,a4);
        for (j = 0; j < i - 1; j++) *out++ = a3[j];
      }
      return (out == (out_begin + decoded_length));
    }
    static int DecodedLength(const char *in, size_t in_length) {
      int numEq = 0;
      const char *in_end = in + in_length;
      while (*--in_end == '=') ++numEq;
      return ((6 * in_length) / 8) - numEq;
    }
    static int DecodedLength(const std::string &in) {
      int numEq = 0;
      int n = in.size();
      for (std::string::const_reverse_iterator it = in.rbegin(); *it == '='; ++it)
        ++numEq;
      return ((6 * n) / 8) - numEq;
    }
    inline static int EncodedLength(size_t length) {
      return (length + 2 - ((length + 2) % 3)) / 3 * 4;
    }
    inline static int EncodedLength(const std::string &in) {
      return EncodedLength(in.length());
    }
    inline static void StripPadding(std::string *in) {
      while (!in->empty() && *(in->rbegin()) == '=') in->resize(in->size() - 1);
    }
   private:
    static inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
      a4[0] = (a3[0] & 0xfc) >> 2;
      a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
      a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
      a4[3] = (a3[2] & 0x3f);
    }
    static inline void a4_to_a3(unsigned char * a3, unsigned char * a4) {
      a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
      a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
      a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
    }
    static inline unsigned char b64_lookup(unsigned char c) {
      if(c >='A' && c <='Z') return c - 'A';
      if(c >='a' && c <='z') return c - 71;
      if(c >='0' && c <='9') return c + 4;
      if(c == '+') return 62;
      if(c == '/') return 63;
      return 255;
    }
  };
}

#endif

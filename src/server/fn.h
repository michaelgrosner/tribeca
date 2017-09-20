#ifndef K_FN_H_
#define K_FN_H_

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

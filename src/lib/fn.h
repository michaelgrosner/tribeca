#ifndef K_FN_H_
#define K_FN_H_

namespace K {
  typedef chrono::duration<int, ratio_multiply<chrono::hours::period, ratio<24>>::type> fnT;
  class FN {
    public:
      static Local<String> v8S(string k) {
        return String::NewFromUtf8(Isolate::GetCurrent(), k.data());
      };
      static Local<String> v8S(Isolate* isolate, string k) {
        return String::NewFromUtf8(isolate, k.data());
      };
      static string S8v(Local<String> k) {
        return string(*String::Utf8Value(k));
      };
      static string S2l(string k) {
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        return k;
      };
      static string S2u(string k) {
        transform(k.begin(), k.end(), k.begin(), ::toupper);
        return k;
      };
      static int S2mC(string k);
      static double T() {
        return chrono::milliseconds(chrono::seconds(time(NULL))).count();
      };
      static string uiT() {
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
        T << hours.count() << ":" << minutes.count() << ":" << seconds.count()
          << "." << milliseconds.count() << microseconds.count() << " ";
        return T.str();
      };
      static string oSha512(string k) {
        unsigned char digest[SHA512_DIGEST_LENGTH];
        SHA512((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[SHA512_DIGEST_LENGTH*2+1];
        for(int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oMd5(string k) {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[33];
        for(int i = 0; i < 16; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return S2u(k_);
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
          if(r != CURLE_OK) cout << "CURL wGet failed " << curl_easy_strerror(r) << endl;;
          curl_easy_cleanup(curl);
        }
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
          if(r != CURLE_OK) cout << "CURL wPost failed " << curl_easy_strerror(r) << endl;;
          curl_easy_cleanup(curl);
        }
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
  };
}

#endif

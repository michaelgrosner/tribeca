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
      static json wJet(string k) {
        return json::parse(wGet(k));
      };
      static string wGet(string k) {
        string k_;
        CURL* curl;
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, k.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
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

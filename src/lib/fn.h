#ifndef K_FN_H_
#define K_FN_H_

namespace K {
  typedef chrono::duration<int, ratio_multiply<chrono::hours::period, ratio<24>>::type> fnT;
  class FN {
    public:
      static Local<String> v8S(string k) {
        return String::NewFromUtf8(Isolate::GetCurrent(), k.data());
      }
      static Local<String> v8S(Isolate* isolate, string k) {
        return String::NewFromUtf8(isolate, k.data());
      }
      static string S8v(Local<String> k) {
        return string(*String::Utf8Value(k));
      }
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
      }
  };
}

#endif

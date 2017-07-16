#ifndef K_FN_H_
#define K_FN_H_

namespace K {
  class FN {
    public:
      static Local<String> v8S(string k) {
        return String::NewFromUtf8(Isolate::GetCurrent(), k.data());
      }
      static string S8v(Local<String> k) {
        return string(*String::Utf8Value(k));
      }
  };
}

#endif

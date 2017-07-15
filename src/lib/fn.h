#ifndef K_FN_H_
#define K_FN_H_

namespace K {
  class FN {
    public:
      static Local<String> v8S(string k) {
        Isolate* isolate = Isolate::GetCurrent();
        return String::NewFromUtf8(isolate, k.data());
      }
      static string S8v(Local<String> k) {
        return string(*String::Utf8Value(k));
      }
  };
}

#endif

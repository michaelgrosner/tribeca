#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  struct Ev { map<string, Persistent<Function>> cb; } ev;
  class EV {
    public:
      static void main(Local<Object> exports) {
        Ev *ev = new Ev;
        NODE_SET_METHOD(exports, "evOn", EV::evOn);
        NODE_SET_METHOD(exports, "evUp", EV::evUp);
      }
    private:
      static void evOn(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        string k = string(*String::Utf8Value(args[0]->ToString()));
        int kc = 0;
        for (map<string, Persistent<Function>>::iterator it=ev.cb.begin(); it!=ev.cb.end(); ++it)
          if (it->first.length() > k.length() && it->first.substr(0, k.length()) == k) kc++;
        k = k.append(".").append(to_string(kc));
        Persistent<Function> *cb = &ev.cb[k];
        cb->Reset(isolate, Local<Function>::Cast(args[1]));
      };
      static void evUp(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        string k = string(*String::Utf8Value(args[0]->ToString())).append(".");
        Local<Value> argv[] = {args[1]->IsUndefined() ? (Local<Value>)Undefined(isolate) : args[1]};
        for (map<string, Persistent<Function>>::iterator it=ev.cb.begin(); it!=ev.cb.end(); ++it)
          if (it->first.length() > k.length() && it->first.substr(0, k.length()) == k) {
            Local<Function>::New(isolate, it->second)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
          }
      };
  };
}

#endif

#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  struct Ev { map<string, vector<CopyablePersistentTraits<Function>::CopyablePersistent>> cb; } ev;
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
        Persistent<Function> cb;
        cb.Reset(isolate, Local<Function>::Cast(args[1]));
        ev.cb[k].push_back(cb);
      };
      static void evUp(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        string k = string(*String::Utf8Value(args[0]->ToString()));
        if (ev.cb.find(k) == ev.cb.end()) return;
        Local<Value> argv[] = {args[1]};
        for (vector<CopyablePersistentTraits<Function>::CopyablePersistent>::iterator cb = ev.cb[k].begin(); cb != ev.cb[k].end(); ++cb)
          Local<Function>::New(isolate, *cb)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
      };
  };
}

#endif

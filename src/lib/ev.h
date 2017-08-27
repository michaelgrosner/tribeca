#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  typedef void (*evCb)(json);
  struct Ev { map<string, vector<CopyablePersistentTraits<Function>::CopyablePersistent>> _cb; map<string, vector<evCb>> cb; } static ev;
  class EV {
    public:
      static void main(Local<Object> exports) {
        signal(SIGSEGV, debug);
        signal(SIGABRT, debug);
        Ev *ev = new Ev;
        NODE_SET_METHOD(exports, "evOn", EV::_evOn);
        NODE_SET_METHOD(exports, "evUp", EV::_evUp);
      }
      static void evOn(string k, evCb cb) {
        ev.cb[k].push_back(cb);
      };
      static void evUp(string k) {
        if (ev.cb.find(k) != ev.cb.end()) {
          for (vector<evCb>::iterator cb = ev.cb[k].begin(); cb != ev.cb[k].end(); ++cb)
            (*cb)({});
        }
        if (ev._cb.find(k) != ev._cb.end()) {
          Isolate* isolate = Isolate::GetCurrent();
          HandleScope scope(isolate);
          Local<Object> o = Object::New(isolate);
          Local<Value> argv[] = {o};
          for (vector<CopyablePersistentTraits<Function>::CopyablePersistent>::iterator _cb = ev._cb[k].begin(); _cb != ev._cb[k].end(); ++_cb)
            Local<Function>::New(isolate, *_cb)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        }
      };
      static void evUp(string k, json o) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        if (ev.cb.find(k) != ev.cb.end()) {
          for (vector<evCb>::iterator cb = ev.cb[k].begin(); cb != ev.cb[k].end(); ++cb)
            (*cb)(o);
        }
        if (ev._cb.find(k) != ev._cb.end()) {
          JSON Json;
          Local<Value> argv[] = {(Local<Value>)Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, o.dump())).ToLocalChecked()};
          for (vector<CopyablePersistentTraits<Function>::CopyablePersistent>::iterator _cb = ev._cb[k].begin(); _cb != ev._cb[k].end(); ++_cb)
            Local<Function>::New(isolate, *_cb)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        }
      };
    private:
      static void debug(int sig) {
        void *array[10];
        size_t size = backtrace(array, 10);
        cerr << FN::uiT() << "Errrror signal " << sig << " "  << strsignal(sig) << endl;
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        cerr << endl << FN::uiT() << "Please, copy and paste this error into a github issue."
             << endl << "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new" << endl;
        exit(1);
      };
      static void _evOn(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        string k = FN::S8v(args[0]->ToString());
        Persistent<Function> _cb;
        _cb.Reset(isolate, Local<Function>::Cast(args[1]));
        ev._cb[k].push_back(_cb);
      };
      static void _evUp(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        string k = FN::S8v(args[0]->ToString());
        if (ev._cb.find(k) != ev._cb.end()) {
          Local<Value> argv[] = {args[1]};
          for (vector<CopyablePersistentTraits<Function>::CopyablePersistent>::iterator _cb = ev._cb[k].begin(); _cb != ev._cb[k].end(); ++_cb)
            Local<Function>::New(isolate, *_cb)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
        }
        if (ev.cb.find(k) != ev.cb.end()) {
          JSON Json;
          json o = args[1]->IsUndefined() ? json{} : json::parse(FN::S8v(Json.Stringify(isolate->GetCurrentContext(), args[1]->ToObject()).ToLocalChecked()));
          for (vector<evCb>::iterator cb = ev.cb[k].begin(); cb != ev.cb[k].end(); ++cb)
            (*cb)(o);
        }
      };
  };
}

#endif

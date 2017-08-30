#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  static uv_timer_t exit_t;
  static void (*evExit)(int code);
  typedef void (*evCb)(json);
  static map<string, vector<evCb>> ev;
  class EV {
    public:
      static void main(Local<Object> exports) {
        evExit = happyEnding;
        signal(SIGINT, quit);
        signal(SIGSEGV, wtf);
        signal(SIGABRT, wtf);
      }
      static void evOn(string k, evCb cb) { ev[k].push_back(cb); };
      static void evUp(string k) { evUp(k, {}); };
      static void evUp(string k, json o) {
        if (ev.find(k) != ev.end())
          for (vector<evCb>::iterator cb = ev[k].begin(); cb != ev[k].end(); ++cb)
            (*cb)(o);
      };
      static void end(int code, int T) {
        if (uv_timer_init(uv_default_loop(), &exit_t)) { cout << FN::uiT() << "Errrror: EV exit_t init failed." << endl; exit(1); }
        exit_t.data = &code;
        if (uv_timer_start(&exit_t, [](uv_timer_t *handle) {
          int* code = (int*)handle->data;
          cout << FN::uiT() << "K exit code " << to_string(*code) << "." << endl;
          exit(*code);
        }, T, 0)) { cout << FN::uiT() << "Errrror: EV exit_t start failed." << endl; exit(1); }
      };
    private:
      static void happyEnding(int code) {
        cout << FN::uiT();
        for(int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << endl;
        end(code, 0);
      };
      static void quit(int sig) {
        cout << endl << FN::uiT() << "Excellent decision! ";
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]");
        if (k["/value/joke"_json_pointer].is_string())
          cout << k["/value/joke"_json_pointer].get<string>() << endl;
        evExit(0);
      };
      static void wtf(int sig) {
        void *k[69];
        cout << FN::uiT() << "Errrror: EV signal " << sig << " "  << strsignal(sig) << " (Three-Headed Monkey found):" << endl;
        backtrace_symbols_fd(k, backtrace(k, 69), STDERR_FILENO);
        cout << endl << "Yikes!"
             << endl << "please copy and paste the error above into a new github issue (noworry for duplicates)."
             << endl << "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
             << endl << endl;
        evExit(1);
      };
  };
}

#endif

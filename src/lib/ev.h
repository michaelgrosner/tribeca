#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  typedef void (*evCb)(json);
  static map<unsigned int, vector<evCb>> ev;
  static void (*evExit)(int code);
  class EV {
    public:
      static void main() {
        evExit = happyEnding;
        signal(SIGINT, quit);
        signal(SIGSEGV, wtf);
        signal(SIGABRT, wtf);
        gitReversedVersion();
      };
      static void on(mEv k, evCb cb) {
        ev[(unsigned int)k].push_back(cb);
      };
      static void up(mEv k, json o = {}) {
        unsigned int kEv = (unsigned int)k;
        if (ev.find(kEv) == ev.end()) return;
        vector<evCb>::iterator cb = ev[kEv].begin();
        for (;cb != ev[kEv].end(); ++cb)
          (*cb)(o);
      };
      static void end(int code) {
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << endl;
        exit(code);
      };
    private:
      static void gitReversedVersion() {
        system("git fetch");
        string k = changelog();
        int commits = count(k.begin(), k.end(), '\n');
        cout << "K version: " << (!commits ? "0day.\n"
          : string("-").append(to_string(commits)).append("commit")
            .append(commits > 1?"s..\n":"..\n").append(k)
        );
      };
      static void happyEnding(int code) {
        cout << FN::uiT();
        for(int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << endl;
        end(EXIT_FAILURE);
      };
      static void quit(int sig) {
        cout << endl << FN::uiT() << "Excellent decision! ";
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]");
        if (k["/value/joke"_json_pointer].is_string())
          cout << k["/value/joke"_json_pointer].get<string>() << endl;
        evExit(EXIT_SUCCESS);
      };
      static void wtf(int sig) {
        cout << FN::uiT() << "Errrror: EV signal " << sig << " "  << strsignal(sig) << " (Three-Headed Monkey found)." << endl;
        if (latest()) report(); else upgrade();
        evExit(EXIT_FAILURE);
      };
      static bool latest() {
        return FN::output("git rev-parse @") == FN::output("git rev-parse @{u}");
      }
      static string changelog() {
        return FN::output("git --no-pager log --oneline @..@{u}");
      }
      static void upgrade() {
        cout << endl << "Hint!"
          << endl << "please upgrade to the latest commit; the encountered error may be already fixed at:"
          << endl << changelog()
          << endl << "If you agree, consider to run \"make latest\" prior further execution."
          << endl << endl;
      };
      static void report() {
        void *k[69];
        backtrace_symbols_fd(k, backtrace(k, 69), STDERR_FILENO);
        cout << endl << "Yikes!"
          << endl << "please copy and paste the error above into a new github issue (noworry for duplicates)."
          << endl << "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
          << endl << endl;
      };
  };
}

#endif

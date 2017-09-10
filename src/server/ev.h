#ifndef K_EV_H_
#define K_EV_H_

#ifndef K_BUILD
#define K_BUILD "0"
#endif

#ifndef K_STAMP
#define K_STAMP "0"
#endif

namespace K {
  typedef void (*evCb)(json);
  static map<unsigned int, vector<evCb>> ev;
  static void (*evExit)(int code);
  class EV {
    public:
      static void main(char** args) {
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
        cout << "K build " << K_BUILD << " " << K_STAMP << "." << endl;
        system("git fetch");
        string k = changelog();
        unsigned int commits = count(k.begin(), k.end(), '\n');
        cout << "K version " << (!commits ? "0day.\n"
          : string("-").append(to_string(commits)).append("commit")
            .append(commits > 1?"s..":"..").append(k)
        );
      };
      static void happyEnding(int code) {
        cout << FN::uiT();
        for(unsigned int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << endl;
        end(EXIT_FAILURE);
      };
      static void quit(int sig) {
        cout << endl;
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]");
        cout << FN::uiT() << "Excellent decision! "
          << k.value("/value/joke"_json_pointer, "let's plant a tree instead..") << endl;
        evExit(EXIT_SUCCESS);
      };
      static void wtf(int sig) {
        cout << FN::uiT() << "Errrror: Signal " << sig << " "  << strsignal(sig);
        if (latest()) {
          cout << " (Three-Headed Monkey found)." << endl;
          report();
          this_thread::sleep_for(chrono::seconds(3));
        } else {
          cout << " (deprecated K version found)." << endl;
          upgrade();
          this_thread::sleep_for(chrono::seconds(21));
        }
        evExit(EXIT_FAILURE);
      };
      static bool latest() {
        return FN::output("git rev-parse @") == FN::output("git rev-parse @{u}");
      }
      static string changelog() {
        return FN::output("git --no-pager log --graph --oneline @..@{u}");
      }
      static void upgrade() {
        cout << endl << "Hint!"
          << endl << "please upgrade to the latest commit; the encountered error may be already fixed at:"
          << endl << changelog()
          << endl << "If you agree, consider to run \"make latest\" prior further executions."
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

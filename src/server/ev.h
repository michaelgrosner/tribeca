#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  uv_timer_t tCalcs,
             tStart,
             tDelay,
             tWallet,
             tCancel;
  int eCode = EXIT_FAILURE;
  class EV: public Klass {
    protected:
      void load() {
        evExit = &happyEnding;
        signal(SIGINT, quit);
        signal(SIGUSR1, wtf);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
      };
      void run() {
        if (FN::output("test -d .git || echo -n zip") == "zip")
          FN::logVer("", -1);
        else {
          FN::output("git fetch");
          string k = changelog();
          FN::logVer(k, count(k.begin(), k.end(), '\n'));
        }
      };
    public:
      function<void(mOrder)>        ogOrder;
      function<void(mTrade)>        ogTrade;
      function<void()>              mgLevels,
                                    mgEwmaSMUProtection,
                                    mgEwmaQuoteProtection,
                                    mgTargetPosition,
                                    pgTargetBasePosition,
                                    uiQuotingParameters;
      void end(int code) {
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << '\n';
        exit(code);
      };
    private:
      function<void(int)> happyEnding = [&](int code) {
        cout << FN::uiT();
        for(unsigned int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << '\n';
        end(code);
      };
      static void quit(int sig) {
        FN::screen_quit();
        cout << '\n';
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]");
        cout << FN::uiT() << "Excellent decision! "
          << ((k.is_null() || !k["/value/joke"_json_pointer].is_string())
            ? "let's plant a tree instead.." : k["/value/joke"_json_pointer].get<string>()
          ) << '\n';
        (*evExit)(EXIT_SUCCESS);
      };
      static void wtf(int sig) {
        FN::screen_quit();
        cout << FN::uiT() << RCYAN << "Errrror: Signal " << sig << " "  << strsignal(sig);
        if (latest()) {
          cout << " (Three-Headed Monkey found)." << '\n';
          report();
          this_thread::sleep_for(chrono::seconds(3));
        } else {
          cout << " (deprecated K version found)." << '\n';
          upgrade();
          this_thread::sleep_for(chrono::seconds(21));
        }
        (*evExit)(EXIT_FAILURE);
      };
      static bool latest() {
        return FN::output("test -d .git && git rev-parse @") == FN::output("test -d .git && git rev-parse @{u}");
      }
      static string changelog() {
        return FN::output("test -d .git && git --no-pager log --graph --oneline @..@{u}");
      }
      static void upgrade() {
        cout << '\n' << BYELLOW << "Hint!" << RYELLOW
          << '\n' << "please upgrade to the latest commit; the encountered error may be already fixed at:"
          << '\n' << changelog()
          << '\n' << "If you agree, consider to run \"make latest\" prior further executions."
          << '\n' << '\n';
      };
      static void report() {
        void *k[69];
        backtrace_symbols_fd(k, backtrace(k, 69), STDERR_FILENO);
        cout << '\n' << BRED << "Yikes!" << RRED
          << '\n' << "please copy and paste the error above into a new github issue (noworry for duplicates)."
          << '\n' << "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
          << '\n' << '\n';
      };
  };
}

#endif

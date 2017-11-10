#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  extern void(*ev_gwDataOrder)          (mOrder);
  extern void(*ev_gwDataTrade)          (mTrade);
  extern void(*ev_gwDataWallet)         (mWallet);
  extern void(*ev_gwDataLevels)         (mLevels);
  extern void(*ev_gwConnectOrder)       (mConnectivity);
  extern void(*ev_gwConnectMarket)      (mConnectivity);
  static void(*ev_gwConnectButton)      (mConnectivity);
  static void(*ev_gwConnectExchange)    (mConnectivity);
  static void(*ev_ogOrder)              (mOrder);
  static void(*ev_ogTrade)              (mTrade);
  static void(*ev_mgLevels)             ();
  static void(*ev_mgEwmaSMUProtection)  ();
  static void(*ev_mgEwmaQuoteProtection)();
  static void(*ev_mgTargetPosition)     ();
  static void(*ev_pgTargetBasePosition) ();
  static void(*ev_uiQuotingParameters)  ();
  static uv_timer_t tCalcs,
                    tStart,
                    tDelay,
                    tWallet,
                    tCancel;
  static int eCode = EXIT_FAILURE;
  class EV: public Klass {
    protected:
      void load() {
        evExit = happyEnding;
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
      static void end(int code) {
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << '\n';
        exit(code);
      };
    private:
      static void quit(int sig) {
        FN::screen_quit();
        cout << '\n';
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]");
        cout << FN::uiT() << "Excellent decision! "
          << ((k.is_null() || !k["/value/joke"_json_pointer].is_string())
            ? "let's plant a tree instead.." : k["/value/joke"_json_pointer].get<string>()
          ) << '\n';
        evExit(EXIT_SUCCESS);
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
        evExit(EXIT_FAILURE);
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
      static void happyEnding(int code) {
        cout << FN::uiT();
        for(unsigned int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << '\n';
        end(code);
      };
  };
}

#endif

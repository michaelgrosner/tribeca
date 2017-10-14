#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  typedef void (*evConnect)      (mConnectivity);
  typedef void (*evOrder)        (mOrder);
  typedef void (*evTrade)        (mTrade);
  typedef void (*evWallet)       (mWallet);
  typedef void (*evLevels)       (mLevels);
  typedef void (*evEmpty)        ();
  extern evConnect       ev_gwConnectButton,
                         ev_gwConnectOrder,
                         ev_gwConnectMarket,
                         ev_gwConnectExchange;
  extern evOrder         ev_gwDataOrder,
                         ev_ogOrder;
  extern evTrade         ev_gwDataTrade,
                         ev_ogTrade;
  extern evWallet        ev_gwDataWallet;
  extern evLevels        ev_gwDataLevels;
  extern evEmpty         ev_mgLevels,
                         ev_mgEwmaQuoteProtection,
                         ev_mgTargetPosition,
                         ev_pgTargetBasePosition,
                         ev_uiQuotingParameters;
  class EV {
    public:
      static void main() {
        evExit = happyEnding;
        signal(SIGINT, quit);
        signal(SIGUSR1, wtf);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
        gitReversedVersion();
      };
      static void end(int code) {
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << '\n';
        exit(code);
      };
    private:
      static void gitReversedVersion() {
        FN::output("git fetch");
        string k = changelog();
        FN::logVer(k, count(k.begin(), k.end(), '\n'));
      };
      static void happyEnding(int code) {
        cout << FN::uiT();
        for(unsigned int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << '\n';
        end(EXIT_FAILURE);
      };
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
        return FN::output("git rev-parse @") == FN::output("git rev-parse @{u}");
      }
      static string changelog() {
        return FN::output("git --no-pager log --graph --oneline @..@{u}");
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

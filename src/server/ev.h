#ifndef K_EV_H_
#define K_EV_H_

namespace K {
  static void (*evExit)(int code);
  typedef void (*evConnect)      (mConnectivity);
  typedef void (*evOrder)        (mOrder);
  typedef void (*evTrade)        (mTrade);
  typedef void (*evTradeHydrated)(mTradeHydrated);
  typedef void (*evWallet)       (mWallet);
  typedef void (*evLevels)       (mLevels);
  typedef void (*evEmpty)        ();
  extern evConnect       ev_gwConnectButton,
                         ev_gwConnectOrder,
                         ev_gwConnectMarket,
                         ev_gwConnectExchange;
  extern evOrder         ev_gwDataOrder,
                         ev_ogOrder;
  extern evTrade         ev_gwDataTrade;
  extern evTradeHydrated ev_ogTrade;
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
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << endl;
        exit(code);
      };
    private:
      static void gitReversedVersion() {
        FN::output("git fetch");
        string k = changelog();
        unsigned int commits = count(k.begin(), k.end(), '\n');
        cout << BGREEN << "K" << RGREEN << " version " << (!commits ? "0day.\n"
          : string("-").append(to_string(commits)).append("commit")
            .append(commits > 1?"s..\n":"..\n").append(BYELLOW).append(k)
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
        cout << FN::uiT() << RCYAN << "Errrror: Signal " << sig << " "  << strsignal(sig);
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
        cout << endl << BYELLOW << "Hint!" << RYELLOW
          << endl << "please upgrade to the latest commit; the encountered error may be already fixed at:"
          << endl << changelog()
          << endl << "If you agree, consider to run \"make latest\" prior further executions."
          << endl << endl;
      };
      static void report() {
        void *k[69];
        backtrace_symbols_fd(k, backtrace(k, 69), STDERR_FILENO);
        cout << endl << BRED << "Yikes!" << RRED
          << endl << "please copy and paste the error above into a new github issue (noworry for duplicates)."
          << endl << "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
          << endl << endl;
      };
  };
}

#endif

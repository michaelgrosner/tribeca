#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  class EV: public Klass {
    private:
      uWS::Hub *hub = nullptr;
      int eCode = EXIT_FAILURE;
    public:
      uWS::Group<uWS::SERVER> *uiGroup = nullptr;
      uv_timer_t *tCalcs = nullptr,
                 *tStart = nullptr,
                 *tDelay = nullptr,
                 *tWallet = nullptr,
                 *tCancel = nullptr;
      function<void(mOrder)> ogOrder;
      function<void(mTrade)> ogTrade;
      function<void()>       mgLevels,
                             mgEwmaSMUProtection,
                             mgEwmaQuoteProtection,
                             mgTargetPosition,
                             pgTargetBasePosition,
                             uiQuotingParameters;
    protected:
      void load() {
        evExit = &happyEnding;
        signal(SIGINT, quit);
        signal(SIGUSR1, wtf);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
        gw->hub = hub = new uWS::Hub(0, true);
      };
      void waitTime() {
        uv_timer_init(hub->getLoop(), tCalcs = new uv_timer_t());
        uv_timer_init(hub->getLoop(), tStart = new uv_timer_t());
        uv_timer_init(hub->getLoop(), tDelay = new uv_timer_t());
        uv_timer_init(hub->getLoop(), tWallet = new uv_timer_t());
        uv_timer_init(hub->getLoop(), tCancel = new uv_timer_t());
      };
      void waitData() {
        gw->gwGroup = hub->createGroup<uWS::CLIENT>();
      };
      void waitUser() {
        uiGroup = hub->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
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
      void start() {
        hub->run();
        halt(eCode);
      };
      void stop(int code, function<void()> gwCancelAll) {
        eCode = code;
        if (uv_loop_alive(hub->getLoop())) {
          uv_timer_stop(tCancel);
          uv_timer_stop(tWallet);
          uv_timer_stop(tCalcs);
          uv_timer_stop(tStart);
          uv_timer_stop(tDelay);
          gw->close();
          gw->gwGroup->close();
          gwCancelAll();
          uiGroup->close();
          FN::close(hub->getLoop());
          hub->getLoop()->destroy();
        }
        halt(code);
      };
      void listen(mutex *k, int headless, int port) {
        gw->wsMutex = k;
        if (headless) return;
        string protocol("HTTP");
        if ((access("etc/sslcert/server.crt", F_OK) != -1) and (access("etc/sslcert/server.key", F_OK) != -1)
          and hub->listen(port, uS::TLS::createContext("etc/sslcert/server.crt", "etc/sslcert/server.key", ""), 0, uiGroup)
        ) protocol += "S";
        else if (!hub->listen(port, nullptr, 0, uiGroup))
          FN::logExit("IU", string("Use another UI port number, ")
            + to_string(port) + " seems already in use by:\n"
            + FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(port)),
            EXIT_SUCCESS);
        FN::logUI(protocol, port);
      }
    private:
      void halt(int code) {
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << '\n';
        exit(code);
      };
      function<void(int)> happyEnding = [&](int code) {
        cout << FN::uiT();
        for(unsigned int i = 0; i < 21; ++i)
          cout << "THE END IS NEVER ";
        cout << "THE END" << '\n';
        halt(code);
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

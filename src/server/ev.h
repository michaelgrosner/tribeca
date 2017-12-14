#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  class EV: public Klass {
    private:
      uWS::Hub *hub = nullptr;
      Async *aEngine = nullptr;
      vector<function<void()>> asyncFn;
      future<int> hotkey;
    public:
      uWS::Group<uWS::SERVER> *uiGroup = nullptr;
      Timer *tServer = nullptr,
            *tEngine = nullptr,
            *tClient = nullptr;
      function<void(mOrder)> ogOrder;
      function<void(mTrade)> ogTrade;
      function<void()>       mgLevels,
                             mgEwmaQuoteProtection,
                             mgTargetPosition,
                             pgTargetBasePosition,
                             uiQuotingParameters;
    protected:
      void load() {
        gwEndings.push_back(&happyEnding);
        signal(SIGINT, quit);
        signal(SIGUSR1, wtf);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
        gw->hub = hub = new uWS::Hub(0, true);
      };
      void waitTime() {
        tServer = new Timer(hub->getLoop());
        tEngine = new Timer(hub->getLoop());
        tClient = new Timer(hub->getLoop());
      };
      void waitData() {
        aEngine = new Async(hub->getLoop());
        aEngine->data = this;
        aEngine->start(asyncLoop);
        gw->gwGroup = hub->createGroup<uWS::CLIENT>();
      };
      void waitUser() {
        uiGroup = hub->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        if (((CF*)config)->argNaked) return;
        hotkey = async(launch::async, FN::screen_events);
      };
      void run() {
        if (FN::output("test -d .git || echo -n zip") == "zip")
          FN::logVer("", -1);
        else {
          FN::output("git fetch");
          string k = changelog();
          FN::logVer(k, count(k.begin(), k.end(), '\n'));
        }
        if (((CF*)config)->argDebugEvents) return;
        debug = [&](string k) {};
      };
    public:
      void start() {
        hub->run();
      };
      void stop(function<void()> gwCancelAll) {
        tServer->stop();
        tEngine->stop();
        tClient->stop();
        gw->close();
        gw->gwGroup->close();
        gwCancelAll();
        asyncLoop(aEngine);
        uiGroup->close();
      };
      void listen() {
        string protocol("HTTP");
        if (!((CF*)config)->argWithoutSSL
          and (access("etc/sslcert/server.crt", F_OK) != -1) and (access("etc/sslcert/server.key", F_OK) != -1)
          and hub->listen(((CF*)config)->argPort, uS::TLS::createContext("etc/sslcert/server.crt", "etc/sslcert/server.key", ""), 0, uiGroup)
        ) protocol += "S";
        else if (!hub->listen(((CF*)config)->argPort, nullptr, 0, uiGroup))
          FN::logExit("IU", string("Use another UI port number, ")
            + to_string(((CF*)config)->argPort) + " seems already in use by:\n"
            + FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(((CF*)config)->argPort)),
            EXIT_SUCCESS);
        FN::logUI(protocol, ((CF*)config)->argPort);
      };
      void deferred(function<void()> fn) {
        asyncFn.push_back(fn);
        aEngine->send();
      };
      function<void(string)> debug = [&](string k) {
        FN::log("DEBUG", string("EV ") + k);
      };
    private:
      function<void()> happyEnding = [&]() {
        cout << FN::uiT() << gw->name;
        for(unsigned int i = 0; i < 21; ++i)
          cout << " THE END IS NEVER";
        cout << " THE END." << '\n';
      };
      void (*asyncLoop)(Async*) = [](Async *handle) {
        EV* k = (EV*)handle->data;
        for (vector<function<void()>>::iterator it = k->asyncFn.begin(); it != k->asyncFn.end();) {
          (*it)();
          it = k->asyncFn.erase(it);
        }
        if (k->hotkey.valid() and k->hotkey.wait_for(chrono::nanoseconds(0)) == future_status::ready) {
          int ch = k->hotkey.get();
          if (ch == 'q' or ch == 'Q')
            raise(SIGINT);
        }
      };
      static void halt(int code) {
        for (vector<function<void()>*>::iterator it=gwEndings.begin(); it!=gwEndings.end();++it) (**it)();
        cout << FN::uiT() << "K exit code " << to_string(code) << "." << '\n';
        exit(code);
      };
      static void quit(int sig) {
        FN::screen_quit();
        cout << '\n';
        json k = FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", true);
        cout << FN::uiT() << "Excellent decision! "
          << ((k.is_null() or !k["/value/joke"_json_pointer].is_string())
            ? "let's plant a tree instead.." : k["/value/joke"_json_pointer].get<string>()
          ) << '\n';
        halt(EXIT_SUCCESS);
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
        halt(EXIT_FAILURE);
      };
      static bool latest() {
        return FN::output("test -d .git && git rev-parse @") == FN::output("test -d .git && git rev-parse @{u}");
      };
      static string changelog() {
        return FN::output("test -d .git && git --no-pager log --graph --oneline @..@{u}");
      };
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

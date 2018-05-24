#ifndef K_EV_H_
#define K_EV_H_

#define _debugEvent_ ((EV*)events)->debug(__PRETTY_FUNCTION__);

namespace K  {
  string tracelog;
  class EV: public Klass {
    private:
      uWS::Hub *hub = nullptr;
      uS::Timer *timer = nullptr;
      uS::Async *aEngine = nullptr;
      vector<function<void()>> slowFn;
      unsigned int evT_5m = 0,
                   gwT_countdown = 0;
    protected:
      void load() {
        endingFn.push_back(&happyEnding);
        endingFn.push_back(&neverEnding);
        signal(SIGINT, quit);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
#ifndef _WIN32
        signal(SIGUSR1, wtf);
#endif
        tracelog = "- upstream: " + args.exchange + '\n'
                 + "- currency: " + args.currency + '\n';
        gw->hub = hub = new uWS::Hub(0, true);
      };
      void waitData() {
        aEngine = new uS::Async(hub->getLoop());
        aEngine->setData(this);
        aEngine->start(asyncLoop);
        hub->createGroup<uWS::CLIENT>();
        gw->reconnect = [&](const string &reason) {
          gwT_countdown = 7;
          screen.log(string("GW ") + gw->name, string("WS ") + reason
            + ", reconnecting in " + to_string(gwT_countdown) + "s.");
        };
        gw->log = [&](const string &reason) {
          gwLog(reason);
        };
      };
      void waitTime() {
        timer = new uS::Timer(hub->getLoop());
        timer->setData(this);
        timer->start([](uS::Timer *timer) {
          ((EV*)timer->getData())->timer_1s();
        }, 0, 1e+3);
      };
      void waitUser() {
        if (args.headless) return;
        hub->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
      };
      void run() {
        if (args.debugEvents) return;
        debug = [](const string &k) {};
      };
    public:
      void start(/* KMxTWEpb9ig */) {
        tracelog += string("- roll-out: ") + to_string(_Tstamp_) + '\n';
        if (gw->asyncWs()) gwT_countdown = 1;
        hub->run();
      };
      void stop(const function<void()> &gwCancelAll) {
        timer->stop();
        gw->close();
        hub->getDefaultGroup<uWS::CLIENT>().close();
        gwCancelAll();
        asyncLoop(aEngine);
        hub->getDefaultGroup<uWS::SERVER>().close();
      };
      inline uWS::Hub* listen() {
        if (!args.withoutSSL
          and (access("etc/sslcert/server.crt", F_OK) != -1)
          and (access("etc/sslcert/server.key", F_OK) != -1)
          and hub->listen(
            args.inet, args.port,
            uS::TLS::createContext("etc/sslcert/server.crt",
                                   "etc/sslcert/server.key", ""),
            0, &hub->getDefaultGroup<uWS::SERVER>()
          )
        ) screen.protocol += 'S';
        else if (!hub->listen(args.inet, args.port, nullptr, 0, &hub->getDefaultGroup<uWS::SERVER>())) {
          const string netstat = FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(args.port));
          exit(screen.error("UI", "Unable to listen to UI port number " + to_string(args.port) + ", "
            + (netstat.empty() ? "try another network interface" : "seems already in use by:\n" + netstat)
          ));
        }
        screen.logUI();
        return hub;
      };
      void deferred(const function<void()> &fn) {
        slowFn.push_back(fn);
        aEngine->send();
      };
      function<void(const string&)> debug = [&](const string &k) {
        screen.log("DEBUG", string("EV ") + k);
      };
    private:
      function<void()> happyEnding = [&]() {
        cout << tracelog;
      };
      function<void()> neverEnding = [&]() {
        cout << gw->name;
        for (unsigned int i = 0; i < 21; ++i)
          cout << " THE END IS NEVER";
        cout << " THE END." << '\n';
      };
      void (*asyncLoop)(uS::Async*) = [](uS::Async *const aEngine) {
        EV* k = (EV*)aEngine->getData();
        if (!k->slowFn.empty()) {
          for (function<void()> &it : k->slowFn) it();
          k->slowFn.clear();
        }
        if (gw->waitForData()) aEngine->send();
        screen.waitForUser();
      };
      void async(const function<bool()> &fn) {
        if (fn()) aEngine->send();
      };
      inline void connect() {
        hub->connect(gw->ws, nullptr, {}, 5e+3, &hub->getDefaultGroup<uWS::CLIENT>());
      };
      inline void timer_1s() {
        if (!gwT_countdown)                   engine.timer_1s();
        else if (gwT_countdown-- == 1)        connect();
        if (FN::trueOnce(&gw->refreshWallet)
          or !(evT_5m % 15))                  async(gw->wallet);
        if (!gw->async) {
          if (!(evT_5m % 2))                  async(gw->orders);
          if (!(evT_5m % 3))                  async(gw->levels);
          if (!(evT_5m % 60)) {
                                              async(gw->trades);
                                              client.timer_60s();
          }
        }
        if (qp.delayUI
          and !(evT_5m % qp.delayUI))         client.timer_Xs();
        if (!(++evT_5m % 300)) {
          if (qp.cancelOrdersAuto)            async(gw->cancelAll);
          if (evT_5m >= 300 * (qp.delayUI?:1))
            evT_5m = 0;
        }
      };
      inline void gwLog(const string &reason) {
        deferred([this, reason]() {
          const string name = string(
            reason.find(">>>") != reason.find("<<<")
              ? "DEBUG" : "GW"
          ) + ' ' + gw->name;
          if (reason.find("Error") != string::npos)
            screen.logWar(name, reason);
          else screen.log(name, reason);
        });
      };
      static void halt(const int code) {
        for (function<void()>* &it : endingFn) (*it)();
        if (code == EXIT_FAILURE)
          this_thread::sleep_for(chrono::seconds(3));
        cout << BGREEN << 'K'
             << RGREEN << " exit code "
             << BGREEN << to_string(code)
             << RGREEN << '.'
             << RRESET << '\n';
        exit(code);
      };
      static void quit(const int sig) {
        tracelog = string("Excellent decision! ")
          + FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
              .value("/value/joke"_json_pointer, "let's plant a tree instead..")
          + '\n';
        halt(EXIT_SUCCESS);
      };
      static void wtf(const int sig) {
        const string rollout = tracelog;
        tracelog = string(RCYAN) + "Errrror: Signal " + to_string(sig) + ' '
#ifndef _WIN32
          + strsignal(sig)
#endif
          ;
        if (FN::output("test -d .git && git rev-parse @") != FN::output("test -d .git && git rev-parse @{u}"))
          tracelog += string("(deprecated K version found).") + '\n'
            + '\n' + string(BYELLOW) + "Hint!" + string(RYELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + SH::changelog()
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        else {
          tracelog += string("(Three-Headed Monkey found):") + '\n' + rollout
            + "- lastbeat: " + to_string(_Tstamp_) + '\n'
#ifndef _WIN32
            + "- os-uname: " + FN::output("uname -srvm")
            + "- tracelog: " + '\n';
          void *k[69];
          size_t jumps = backtrace(k, 69);
          char **trace = backtrace_symbols(k, jumps);
          size_t i;
          for (i = 0; i < jumps; i++)
            tracelog += string(trace[i]) + '\n';
          free(trace)
#endif
          ;
          tracelog += '\n' + string(BRED) + "Yikes!" + string(RRED)
            + '\n' + "please copy and paste the error above into a new github issue (noworry for duplicates)."
            + '\n' + "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
            + '\n' + '\n';
        }
        halt(EXIT_FAILURE);
      };
  };
}

#endif

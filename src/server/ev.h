#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  class EV: public Klass,
            public Events { public: EV() { events = this; };
    private:
      uWS::Hub  *hub   = nullptr;
      uS::Timer *timer = nullptr;
      uS::Async *loop  = nullptr;
      vector<function<void()>> slowFn;
      unsigned int evT_5m        = 0,
                   gwT_countdown = 0;
    protected:
      void load() {
        endingFn.push_back(&happyEnding);
        endingFn.push_back(&neverEnding);
        gw->hub = hub = new uWS::Hub(0, true);
      };
      void waitData() {
        hub->createGroup<uWS::CLIENT>();
        gw->reconnect = [&](const string &reason) {
          gwT_countdown = 7;
          screen->log(string("GW ") + gw->name, string("WS ") + reason
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
        loop = new uS::Async(hub->getLoop());
        loop->setData(this);
        loop->start(walk);
      };
    public:
      uWS::Hub* listen() {
        if (!args.withoutSSL
          and (access("etc/sslcert/server.crt", F_OK) != -1)
          and (access("etc/sslcert/server.key", F_OK) != -1)
          and hub->listen(
            args.inet, args.port,
            uS::TLS::createContext("etc/sslcert/server.crt",
                                   "etc/sslcert/server.key", ""),
            0, &hub->getDefaultGroup<uWS::SERVER>()
          )
        ) screen->logUI("HTTPS");
        else if (!hub->listen(args.inet, args.port, nullptr, 0, &hub->getDefaultGroup<uWS::SERVER>())) {
          const string netstat = FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(args.port));
          exit(screen->error("UI", "Unable to listen to UI port number " + to_string(args.port) + ", "
            + (netstat.empty() ? "try another network interface" : "seems already in use by:\n" + netstat)
          ));
        } else screen->logUI("HTTP");
        return hub;
      };
      void start(/* KMxTWEpb9ig */) {
        tracelog += string("- roll-out: ") + to_string(_Tstamp_) + '\n';
        if (gw->asyncWs()) gwT_countdown = 1;
        hub->run();
      };
      void stop() {
        timer->stop();
        gw->close();
        hub->getDefaultGroup<uWS::CLIENT>().close();
        gw->clear();
        walk(loop);
        hub->getDefaultGroup<uWS::SERVER>().close();
      };
      void deferred(const function<void()> &fn) {
        slowFn.push_back(fn);
        loop->send();
      };
    private:
      function<void()> happyEnding = [&]() {
        cout << tracelog;
        tracelog.clear();
      };
      function<void()> neverEnding = [&]() {
        cout << gw->name;
        for (unsigned int i = 0; i < 21; ++i)
          cout << " THE END IS NEVER";
        cout << " THE END." << '\n';
      };
      void (*walk)(uS::Async*) = [](uS::Async *const loop) {
        EV* k = (EV*)loop->getData();
        if (!k->slowFn.empty()) {
          for (function<void()> &it : k->slowFn) it();
          k->slowFn.clear();
        }
        if (gw->waitForData()) loop->send();
        screen->waitForUser();
      };
      void async(const function<bool()> &fn) {
        if (fn()) loop->send();
      };
      inline void connect() {
        hub->connect(gw->ws, nullptr, {}, 5e+3, &hub->getDefaultGroup<uWS::CLIENT>());
      };
      inline void timer_1s() {
        if (!gwT_countdown)                   engine->timer_1s();
        else if (gwT_countdown-- == 1)        connect();
        if (FN::trueOnce(&gw->refreshWallet)
          or !(evT_5m % 15))                  async(gw->wallet);
        if (!gw->async) {
          if (!(evT_5m % 2))                  async(gw->orders);
          if (!(evT_5m % 3))                  async(gw->levels);
          if (!(evT_5m % 60))                 async(gw->trades);
        }
        if (!(evT_5m % 60))                   client->timer_60s();
        if (!args.headless and qp.delayUI
          and !(evT_5m % qp.delayUI))         client->timer_Xs();
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
            screen->logWar(name, reason);
          else screen->log(name, reason);
        });
      };
  };
}

#endif

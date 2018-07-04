#ifndef K_EV_H_
#define K_EV_H_

namespace K  {
  class EV: public Klass,
            public Events { public: EV() { events = this; };
    private:
      uWS::Hub  *socket = nullptr;
      uS::Timer *timer  = nullptr;
      uS::Async *loop   = nullptr;
      vector<function<void()>> slowFn;
      unsigned int tick = 0;
    protected:
      void load() {
        socket = new uWS::Hub(0, true);
      };
      void waitData() {
        gw->socket = socket;
        socket->createGroup<uWS::CLIENT>();
      };
      void waitWebAdmin() {
        client->socket = socket;
        socket->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
      };
      void waitSysAdmin() {
        screen->pressme(mHotkey::Q, [&]() { raise(SIGINT); });
        screen->pressme(mHotkey::q, [&]() { raise(SIGINT); });
      };
      void waitTime() {
        timer = new uS::Timer(socket->getLoop());
        timer->setData(this);
        timer->start([](uS::Timer *timer) {
          ((EV*)timer->getData())->timer_1s();
        }, 0, 1e+3);
      };
      void run() {
        loop = new uS::Async(socket->getLoop());
        loop->setData(this);
        loop->start(walk);
      };
      void end() {
        timer->stop();
        gw->close();
        socket->getDefaultGroup<uWS::CLIENT>().close();
        gw->clear();
        walk(loop);
        socket->getDefaultGroup<uWS::SERVER>().close();
      };
    public:
      void deferred(const function<void()> &fn) {
        slowFn.push_back(fn);
        loop->send();
      };
    private:
      void async(const function<bool()> &fn) {
        if (fn()) loop->send();
      };
      void (*walk)(uS::Async*) = [](uS::Async *const loop) {
        EV *k = (EV*)loop->getData();
        if (!k->slowFn.empty()) {
          for (function<void()> &it : k->slowFn) it();
          k->slowFn.clear();
        }
        if (gw->waitForData()) loop->send();
        screen->waitForUser();
      };
      void timer_1s() {
        if (!gw->countdown) {
          if (!market->levels.warn_empty()) {
                                             market->levels.timer_1s();
            if (!(tick % 60))                market->levels.timer_60s();
                                             wallet->timer_1s();
                                             engine->timer_1s();
          }
        } else if (gw->countdown-- == 1) {   gw->connect();
          tick = 0;
          return;
        }
        if (TRUEONCE(gw->refreshWallet)
          or !(tick % 15))                   async(gw->wallet);
        if (!gw->async) {
          if (!(tick % 2))                   async(gw->orders);
          if (!(tick % 3))                   async(gw->levels);
          if (!(tick % 60))                  async(gw->trades);
        }
        if (!(tick % 60))                    engine->monitor.timer_60s();
        if (client->socket and qp.delayUI
          and !(tick % qp.delayUI))          client->timer_Xs();
        if (!(++tick % 300)) {
          if (qp.cancelOrdersAuto)           async(gw->cancelAll);
          if (tick >= 300 * (qp.delayUI?:1))
            tick = 0;
        }
      };
  };
}

#endif

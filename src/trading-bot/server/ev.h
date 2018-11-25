#ifndef K_EV_H_
#define K_EV_H_

class EV: public Events { public: EV() { events = this; };
  private:
    uWS::Hub  *socket = nullptr;
    uS::Timer *timer  = nullptr;
    uS::Async *loop   = nullptr;
    vector<function<void()>> slowFn;
    unsigned int tick = 0;
  protected:
    void load() {
      socket = new uWS::Hub(0, true);
      K.screen.log("CF", "Network Interface for outgoing traffic is",
        client->wtfismyip = mREST::xfer("https://wtfismyip.com/json", 4L)
                              .value("YourFuckingIPAddress", client->wtfismyip)
      );
    };
    void waitData() {
      gw->socket = socket;
      socket->createGroup<uWS::CLIENT>();
    };
    void waitWebAdmin() {
      if (K.option.num("headless")) return;
      client->socket = socket;
      socket->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
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
      gw->end(K.option.num("dustybot"));
      walk(loop);
      socket->getDefaultGroup<uWS::SERVER>().close();
    };
  public:
    void deferred(const function<void()> &fn) {
      slowFn.push_back(fn);
      loop->send();
    };
  private:
    void deferred() {
      for (function<void()> &it : slowFn) it();
      slowFn.clear();
    };
    void (*walk)(uS::Async *const) = [](uS::Async *const loop) {
      ((EV*)loop->getData())->deferred();
      if (gw->waitForData()) loop->send();
      K.screen.waitForUser();
    };
    void timer_1s() {
      if (!gw->countdown)                  engine->timer_1s(tick);
      else if (!--gw->countdown) {         gw->connect();
        tick = 0;
        return;
      }
      if (                                 gw->askForData(tick)
      ) loop->send();
      if (client->socket and qp.delayUI
        and !(tick % qp.delayUI))          client->timer_Xs();
      if (++tick >= 300 * (qp.delayUI?:1))
        tick = 0;
    };
} ev;

#endif

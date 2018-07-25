#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW: public Klass {
    protected:
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          if (!gw->semaphore.online(rawdata))
            gw->levels.clear();
        });
      };
      void waitWebAdmin() {
        client->welcome(gw->notepad);
        client->clickme(gw->notepad);
        client->welcome(gw->monitor);
        client->welcome(gw->monitor.product);
        client->welcome(gw->semaphore);
        client->clickme(gw->semaphore KISS {
          if (!butterfly.is_number()) return;
          gw->semaphore.agree(butterfly.get<mConnectivity>());
        });
      };
      void waitSysAdmin() {
        screen->printme(gw);
        screen->printme(&gw->semaphore);
        screen->pressme(mHotkey::ESC, [&]() {
          gw->semaphore.toggle();
        });
      };
      void run() {                                                  PRETTY_DEBUG
        const string msg = gw->config_externals();
        if (!msg.empty())
          EXIT(screen->error("GW", msg));
      };
  };
}

#endif

#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW: public Klass {
    protected:
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          if (!engine->semaphore.online(rawdata))
            engine->levels.clear();
        });
      };
      void waitWebAdmin() {
        client->welcome(engine->notepad);
        client->clickme(engine->notepad);
        client->welcome(gw->monitor);
        client->welcome(gw->monitor.product);
        client->welcome(engine->semaphore);
        client->clickme(engine->semaphore);
      };
      void waitSysAdmin() {
        screen->printme(&engine->semaphore);
        screen->pressme(mHotkey::ESC, [&]() {
          engine->semaphore.toggle();
        });
      };
      void run() {                                                  PRETTY_DEBUG
        const string msg = gw->load_externals();
        if (!msg.empty())
          EXIT(screen->error("GW", msg));
      };
  };
}

#endif

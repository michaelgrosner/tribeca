#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW: public Klass {
    private:
      mConnectivity adminAgreement = mConnectivity::Disconnected;
    protected:
      void load() {
        adminAgreement = (mConnectivity)args.autobot;
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          if (gw->semaphore.greenGateway == rawdata) return;
          if (!(gw->semaphore.greenGateway = rawdata))
            gw->levels.clear();
          gwSemaphore();
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
          mConnectivity k = butterfly.get<mConnectivity>();
          if (adminAgreement != k) {
            adminAgreement = k;
            gwSemaphore();
          }
        });
      };
      void waitSysAdmin() {
        screen->printme(gw);
        screen->printme(&gw->semaphore);
        screen->pressme(mHotkey::ESC, [&]() {
          adminAgreement = (mConnectivity)!adminAgreement;
          gwSemaphore();
        });
      };
      void run() {                                                  PRETTY_DEBUG
        const string msg = gw->config_externals();
        if (!msg.empty())
          EXIT(screen->error("GW", msg));
        if (gw->exchange == mExchange::Coinbase) mCommand::stunnel(true);
      };
      void end() {
        if (gw->exchange == mExchange::Coinbase) mCommand::stunnel(false);
      };
    private:
      void gwSemaphore() {
        mConnectivity k = adminAgreement * gw->semaphore.greenGateway;
        if (gw->semaphore.greenButton != k) {
          gw->semaphore.greenButton = k;
          screen->log("GW " + gw->name, "Quoting state changed to",
            string(!gw->semaphore.greenButton?"DIS":"") + "CONNECTED");
        }
        gw->semaphore.send();
        gw->semaphore.refresh();
      };
  };
}

#endif

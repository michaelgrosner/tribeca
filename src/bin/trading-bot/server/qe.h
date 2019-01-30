#ifndef K_QE_H_
#define K_QE_H_

class QE: public Engine { public: QE() { engine = this; };
  protected:
    void load() override {
      {
        SQLITE_BACKUP
      } {
        K.gateway->askForCancelAll = &qp.cancelOrdersAuto;
        client->delay              = &qp.delayUI;
      } {
        K.timer_ticks_factor(qp.delayUI);
        broker.calculon.dummyMM.mode("loaded");
      } {
        broker.semaphore.agree(K.num("autobot"));
        K.timer_1s([&](const unsigned int &tick) {
          if (!K.gateway->countdown) timer_1s(tick);
          return false;
        });
      }
    };
    void waitData() override {
      K.gateway->write_Connectivity = [&](const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
        if (broker.semaphore.offline())
          levels.clear();
      };
      K.gateway->write_mWallets = [&](const mWallets &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      K.gateway->write_mLevels = [&](const mLevels &rawdata) {
        levels.read_from_gw(rawdata);
        wallet.calcFunds();
        calcQuotes();
      };
      K.gateway->write_mOrder = [&](const mOrder &rawdata) {
        orders.read_from_gw(rawdata);
        wallet.calcFundsAfterOrder(orders.updated, &K.gateway->askForFees);
      };
      K.gateway->write_mTrade = [&](const mTrade &rawdata) {
        levels.stats.takerTrades.read_from_gw(rawdata);
      };
    };
    void waitWebAdmin() override {
      CLIENT_WELCOME
      CLIENT_CLICKME
    };
    void waitSysAdmin() override {
      HOTKEYS
    };
    void run() override {
      K.handshake({
        {"gateway", K.gateway->http },
        {"gateway", K.gateway->ws   },
        {"gateway", K.gateway->fix  },
        {"autoBot", K.num("autobot")
                      ? "yes"
                      : "no"        }
      });
    };
} qe;

#endif

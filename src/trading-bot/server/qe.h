#ifndef K_QE_H_
#define K_QE_H_

class QE: public Engine { public: QE() { engine = this; };
  protected:
    void load() override {
      SQLITE_BACKUP
      gw->askForCancelAll = &qp.cancelOrdersAuto;
      monitor.unlock          = &gw->unlock;
      monitor.product.minTick = &gw->minTick;
      monitor.product.minSize = &gw->minSize;
      K.timer_ticks_factor(qp.delayUI);
      broker.calculon.dummyMM.mode("loaded");
      broker.semaphore.agree(K.num("autobot"));
      K.timer_1s_online([&](const unsigned int &tick) {
        timer_1s(tick);
      });
    };
    void waitData() override {
      gw->RAWDATA_ENTRY_POINT(Connectivity, {
        broker.semaphore.read_from_gw(rawdata);
        if (broker.semaphore.offline())
          levels.clear();
      });
      gw->RAWDATA_ENTRY_POINT(mWallets, {
        wallet.read_from_gw(rawdata);
      });
      gw->RAWDATA_ENTRY_POINT(mLevels, {
        levels.read_from_gw(rawdata);
        wallet.calcFunds();
        calcQuotes();
      });
      gw->RAWDATA_ENTRY_POINT(mOrder, {
        orders.read_from_gw(rawdata);
        wallet.calcFundsAfterOrder(orders.updated, &gw->askForFees);
      });
      gw->RAWDATA_ENTRY_POINT(mTrade, {
        levels.stats.takerTrades.read_from_gw(rawdata);
      });
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
        {"gateway", gw->http        },
        {"gateway", gw->ws          },
        {"gateway", gw->fix         },
        {"autoBot", K.num("autobot")
                      ? "yes"
                      : "no"        }
      });
    };
} qe;

#endif

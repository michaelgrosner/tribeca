#ifndef K_QE_H_
#define K_QE_H_

class QE: public Engine { public: QE() { engine = this; };
  protected:
    void load() {
      SQLITE_BACKUP
      gw->askForCancelAll = &qp.cancelOrdersAuto;
      monitor.unlock          = &gw->unlock;
      monitor.product.minTick = &gw->minTick;
      monitor.product.minSize = &gw->minSize;
      broker.calculon.dummyMM.mode("loaded");
      broker.semaphore.agree(K.option.num("autobot"));
    };
    void waitData() {
      gw->RAWDATA_ENTRY_POINT(mConnectivity, {
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
    void waitWebAdmin() {
      CLIENT_WELCOME
      CLIENT_CLICKME
    };
    void waitSysAdmin() {
      SCREEN_PRESSME
    };
    void run() {
      K.handshake({
        {"gateway", gw->http               },
        {"gateway", gw->ws                 },
        {"gateway", gw->fix                },
        {"autoBot", K.option.num("autobot")
                      ? "yes"
                      : "no"               }
      });
    };
} qe;

#endif

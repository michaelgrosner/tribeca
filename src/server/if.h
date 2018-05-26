#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  static struct kArgs {
        int port          = 3000,   colors      = 0, debug        = 0,
            debugSecret   = 0,      debugEvents = 0, debugOrders  = 0,
            debugQuotes   = 0,      debugWallet = 0, withoutSSL   = 0,
            headless      = 0,      dustybot    = 0, lifetime     = 0,
            autobot       = 0,      naked       = 0, free         = 0,
            ignoreSun     = 0,      ignoreMoon  = 0, maxLevels    = 0,
            maxAdmins     = 7,      testChamber = 0;
    mAmount maxWallet     = 0;
     mPrice ewmaUShort    = 0,      ewmaXShort  = 0, ewmaShort    = 0,
            ewmaMedium    = 0,      ewmaLong    = 0, ewmaVeryLong = 0;
     string title         = "K.sh", matryoshka  = "https://www.example.com/",
            user          = "NULL", pass        = "NULL",
            exchange      = "NULL", currency    = "NULL",
            apikey        = "NULL", secret      = "NULL",
            username      = "NULL", passphrase  = "NULL",
            http          = "NULL", wss         = "NULL",
            database      = "",     diskdata    = "",
            whitelist     = "";
    const char *inet = nullptr;
  } args;
  static struct kEvents {
    function<void()> start,
                     stop;
    function<void(const function<void()>&)> deferred;
    function<uWS::Hub*()> listen;
  } events;
  static struct kSqlite {
    function<json(const mMatter&)> select;
    inline void insert(
      const mMatter &table            ,
      const json    &cell             ,
      const bool    &rm       = true  ,
      const string  &updateId = "NULL",
      const mClock  &rmOlder  = 0
    ) {
      delete_insert(table, cell, rm, updateId, rmOlder);
    };
    function<void(const mMatter&, const json&, const bool&, const string&, const mClock&)> delete_insert;
    function<unsigned int()> size = []() { return 0; };
  } sqlite;
  static struct kClient {
    function<void()> timer_Xs   = []() {},
                     timer_60s  = []() {};
    function<void(const mMatter&, function<void(json*)>*)>       welcome = [](const mMatter &type, function<void(json*)> *fn) {};
    function<void(const mMatter&, function<void(const json&)>*)> clickme = [](const mMatter &type, function<void(const json&)> *fn) {};
    function<void(const mMatter&, const json&)>                  send;
  } client;
  static struct kWallet {
    mPosition position;
    mSafety safety;
    mAmount targetBasePosition = 0,
            positionDivergence = 0;
    string sideAPR = "";
    function<void()> calcWallet,
                     calcSafety,
                     calcTargetBasePos;
    function<void(const mSide&)> calcWalletAfterOrder;
    function<void(const mTrade&)> calcSafetyAfterTrade;
  } wallet;
  static struct kMarket {
    mLevels levels;
    mPrice fairValue = 0,
           mgEwmaP = 0,
           mgEwmaW = 0;
    double targetPosition = 0,
           mgStdevTop = 0,
           mgStdevTopMean = 0,
           mgStdevFV = 0,
           mgStdevFVMean = 0,
           mgStdevBid = 0,
           mgStdevBidMean = 0,
           mgStdevAsk = 0,
           mgStdevAskMean = 0,
           mgEwmaTrendDiff = 0;
    map<mPrice, mAmount> filterBidOrders,
                         filterAskOrders;
    function<void()> calcStats,
                     calcFairValue,
                     calcEwmaHistory;
  } market;
  static struct kBroker {
    map<mRandId, mOrder> orders;
    vector<mTrade> tradesHistory;
    function<void(const mRandId&)> cleanOrder,
                                   cancelOrder;
    function<void(
            vector<mRandId>  toCancel,
      const mSide           &side    ,
      const mPrice          &price   ,
      const mAmount         &qty     ,
      const mOrderType      &type    ,
      const mTimeInForce    &tif     ,
      const bool            &isPong  ,
      const bool            &postOnly
    )> sendOrder;
  } broker;
  static struct kEngine {
    function<void()> timer_1s,
                     calcQuote,
                     calcQuoteAfterSavedParams;
        unsigned int orders_60s = 0;
       mConnectivity greenButton  = mConnectivity::Disconnected,
                     greenGateway = mConnectivity::Disconnected;
  } engine;
  static class Gw {
    public:
      virtual string A() = 0;
      uWS::Hub *hub = nullptr;
      static Gw *config(mCoinId, mCoinId, string, int, string, string, string, string, string, string, int, int);
      function<void(const string&)> log,
                                    reconnect;
      function<void(mOrder)>        evDataOrder;
      function<void(mTrade)>        evDataTrade;
      function<void(mLevels)>       evDataLevels;
      function<void(mWallets)>      evDataWallet;
      function<void(mConnectivity)> evConnectOrder,
                                    evConnectMarket;
      function<void(mRandId, string)> replace;
      virtual void place(mRandId, mSide, string, string, mOrderType, mTimeInForce, bool, mClock) = 0,
                   cancel(mRandId, mRandId) = 0,
                   close() = 0;
      mRandId (*randId)() = nullptr;
      mExchange exchange = (mExchange)0;
            int version  = 0, maxLevel = 0,
                debug    = 0;
         mPrice minTick  = 0;
        mAmount makeFee  = 0, takeFee  = 0,
                minSize  = 0;
        mCoinId base     = "", quote   = "";
         string name     = "", symbol  = "",
                apikey   = "", secret  = "",
                user     = "", pass    = "",
                ws       = "", http    = "";
      bool refreshWallet = false,
                   async = false;
      virtual bool asyncWs() = 0;
      inline bool waitForData() {
        return waitFor(replyOrders, evDataOrder)
             | waitFor(replyLevels, evDataLevels)
             | waitFor(replyTrades, evDataTrade)
             | waitFor(replyWallets, evDataWallet)
             | waitFor(replyCancelAll, evDataOrder);
      };
      function<bool()> wallet = [&]() { return !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); })); };
      function<bool()> levels = [&]() { return askFor(replyLevels, [&]() { return sync_levels(); }); };
      function<bool()> trades = [&]() { return askFor(replyTrades, [&]() { return sync_trades(); }); };
      function<bool()> orders = [&]() { return askFor(replyOrders, [&]() { return sync_orders(); }); };
      function<bool()> cancelAll = [&]() { return askFor(replyCancelAll, [&]() { return sync_cancelAll(); }); };
      virtual vector<mOrder> sync_cancelAll() = 0;
    protected:
      virtual bool async_wallet() { return false; };
      virtual vector<mWallets> sync_wallet() { return vector<mWallets>(); };
      virtual vector<mLevels> sync_levels() { return vector<mLevels>(); };
      virtual vector<mTrade> sync_trades() { return vector<mTrade>(); };
      virtual vector<mOrder> sync_orders() { return vector<mOrder>(); };
      future<vector<mWallets>> replyWallets;
      future<vector<mLevels>> replyLevels;
      future<vector<mTrade>> replyTrades;
      future<vector<mOrder>> replyOrders;
      future<vector<mOrder>> replyCancelAll;
      template<typename mData, typename syncFn> inline bool askFor(
              future<vector<mData>> &reply,
        const syncFn                &read
      ) {
        bool waiting = reply.valid();
        if (!waiting) {
          reply = ::async(launch::async, read);
          waiting = true;
        }
        return waiting;
      };
      template<typename mData> inline unsigned int waitFor(
              future<vector<mData>> &reply,
        const function<void(mData)> &write
      ) {
        bool waiting = reply.valid();
        if (waiting and reply.wait_for(chrono::nanoseconds(0))==future_status::ready) {
          for (mData &it : reply.get()) write(it);
          waiting = false;
        }
        return waiting;
      };
  } *gw = nullptr;
  class Klass {
    protected:
      virtual void load() {};
      virtual void waitData() {};
      virtual void waitTime() {};
      virtual void waitUser() {};
      virtual void run() {};
    public:
      inline void wait() {
        load();
        waitData();
        waitTime();
        waitUser();
        run();
      };
  };
}

#endif
#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  static struct Screen {
    Screen() {
      cout << BGREEN << "K"
           << RGREEN << " build " << K_BUILD << ' ' << K_STAMP
           << '.' << BRED << '\n';
      string changes;
      int commits = -1;
      if (access(".git", F_OK) != -1) {
        system("git fetch");
        changes = FN::changelog();
        commits = count(changes.begin(), changes.end(), '\n');
      }
      cout << BGREEN << K_0_DAY << RGREEN << ' ' << (commits == -1
        ? "(zip install)"
        : (commits
          ? '-' + to_string(commits) + "commit" + (commits == 1?"":"s") + '.'
          : "(0day)"
        )
      ) << ".\n" << RYELLOW << changes << RWHITE << RRESET;
    };
    virtual void config() = 0;
    virtual void pressme(mHotkey ch, function<void()> *fn) = 0;
    virtual int error(string k, string s, bool reboot = false) = 0;
    virtual void waitForUser() = 0;
    virtual string stamp() = 0;
    virtual void logErr(string k, string s, string m = " Errrror: ") = 0;
    void logWar(string k, string s) {
      logErr(k, s, " Warrrrning: ");
    };
    virtual void logDB(string k) = 0;
    virtual void logUI(const string &protocol_) = 0;
    virtual void logUIsess(int k, string s) = 0;
    virtual void log(mTrade k, string e) = 0;
    virtual void log(string k, string s, string v) = 0;
    virtual void log(string k, string s) = 0;
    virtual void log(string k, int c = COLOR_WHITE, bool b = false) = 0;
    virtual void log(const map<mRandId, mOrder> &orders, bool working) = 0;
    virtual void log(const mPosition &pos) = 0;
    virtual void log(double fv) = 0;
    function<void(const string&)> debug = [&](const string &k) {
      log("DEBUG", string("EV ") + k);
    };
    virtual void refresh() = 0;
    virtual void end() = 0;
  } *screen = nullptr;
  static struct Events {
    virtual uWS::Hub* listen() = 0;
    virtual void start() = 0;
    virtual void deferred(const function<void()> &fn) = 0;
  } *events = nullptr;
  static struct Sqlite {
    virtual json select(const mMatter &table) = 0;
    virtual void insert(
      const mMatter &table            ,
      const json    &cell             ,
      const bool    &rm       = true  ,
      const string  &updateId = "NULL",
      const mClock  &rmOlder  = 0
    ) = 0;
    function<unsigned int()> size = []() { return 0; };
  } *sqlite = nullptr;
  static struct Client {
    virtual void timer_Xs() = 0;
    virtual void timer_60s() = 0;
    function<void(const mMatter&, function<void(json*)>*)>       welcome = [](const mMatter &type, function<void(json*)> *fn) {};
    function<void(const mMatter&, function<void(const json&)>*)> clickme = [](const mMatter &type, function<void(const json&)> *fn) {};
    function<void(const mMatter&, const json&)>                  send;
  } *client = nullptr;
  static struct Wallet {
    mPosition position;
    mSafety safety;
    mAmount targetBasePosition = 0,
            positionDivergence = 0;
    string sideAPR = "";
    virtual void calcWallet() = 0;
    virtual void calcSafety() = 0;
    virtual void calcTargetBasePos() = 0;
    virtual void calcWalletAfterOrder(const mSide&) = 0;
    virtual void calcSafetyAfterTrade(const mTrade&) = 0;
  } *wallet = nullptr;
  static struct Market {
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
    virtual void calcStats() = 0;
    virtual void calcFairValue() = 0;
    virtual void calcEwmaHistory() = 0;
  } *market = nullptr;
  static struct Broker {
    map<mRandId, mOrder> orders;
    vector<mTrade> tradesHistory;
    virtual void cleanOrder(const mRandId&) = 0;
    virtual void cancelOrder(const mRandId&) = 0;
    virtual void sendOrder(
            vector<mRandId>  toCancel,
      const mSide           &side    ,
      const mPrice          &price   ,
      const mAmount         &qty     ,
      const mOrderType      &type    ,
      const mTimeInForce    &tif     ,
      const bool            &isPong  ,
      const bool            &postOnly
    ) = 0;;
  } *broker = nullptr;
  static struct Engine {
    unsigned int orders_60s = 0;
    mConnectivity greenButton  = mConnectivity::Disconnected,
                  greenGateway = mConnectivity::Disconnected;
    virtual void timer_1s() = 0;
    virtual void calcQuote() = 0;
    virtual void calcQuoteAfterSavedParams() = 0;
  } *engine = nullptr;
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
        return (async
          ? false
          : waitFor(replyOrders, evDataOrder)
            | waitFor(replyLevels, evDataLevels)
            | waitFor(replyTrades, evDataTrade)
        ) | waitFor(replyWallets, evDataWallet)
          | waitFor(replyCancelAll, evDataOrder);
      };
      function<bool()> wallet = [&]() { return !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); })); };
      function<bool()> levels = [&]() { return askFor(replyLevels, [&]() { return sync_levels(); }); };
      function<bool()> trades = [&]() { return askFor(replyTrades, [&]() { return sync_trades(); }); };
      function<bool()> orders = [&]() { return askFor(replyOrders, [&]() { return sync_orders(); }); };
      function<bool()> cancelAll = [&]() { return askFor(replyCancelAll, [&]() { return sync_cancelAll(); }); };
      virtual vector<mOrder> sync_cancelAll() = 0;
      inline void clear() {
        if (args.dustybot)
          screen->log(string("GW ") + name, "--dustybot is enabled, remember to cancel manually any open order.");
        else {
          screen->log(string("GW ") + name, "Attempting to cancel all open orders, please wait.");
          for (mOrder &it : sync_cancelAll()) evDataOrder(it);
          screen->log(string("GW ") + name, "cancel all open orders OK");
        }
      };
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
  static string tracelog;
  static vector<function<void()>> happyEndingFn, endingFn = {
    [](){
      screen->end();
      cout << '\n' << screen->stamp() << tracelog;
    }
  };
  static class Ending {
    public:
      Ending (/* KMxTWEpb9ig */) {
        tracelog = "- roll-out: " + to_string(_Tstamp_) + '\n';
        signal(SIGINT, quit);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
#ifndef _WIN32
        signal(SIGUSR1, wtf);
#endif
      };
    private:
      static void halt(const int code) {
        endingFn.swap(happyEndingFn);
        for (function<void()> &it : happyEndingFn) it();
        if (code == EXIT_FAILURE)
          this_thread::sleep_for(chrono::seconds(3));
        cout << BGREEN << 'K'
             << RGREEN << " exit code "
             << BGREEN << to_string(code)
             << RGREEN << '.'
             << RRESET << '\n';
        exit(code);
      };
      static void quit(const int sig) {
        tracelog = "Excellent decision! "
          + FN::wJet("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
              .value("/value/joke"_json_pointer, "let's plant a tree instead..")
          + '\n';
        halt(EXIT_SUCCESS);
      };
      static void wtf(const int sig) {
        const string rollout = tracelog;
        tracelog = string(RCYAN) + "Errrror: Signal " + to_string(sig) + ' '
#ifndef _WIN32
          + strsignal(sig)
#endif
          + ' ';
        if (FN::output("test -d .git && git rev-parse @") != FN::output("test -d .git && git rev-parse @{u}"))
          tracelog += string("(deprecated K version found).") + '\n'
            + '\n' + string(BYELLOW) + "Hint!" + string(RYELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + FN::changelog()
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        else {
          tracelog += string("(Three-Headed Monkey found):") + '\n'
            + "- exchange: " + args.exchange + '\n'
            + "- currency: " + args.currency + '\n'
            + rollout
            + "- lastbeat: " + to_string(_Tstamp_) + '\n'
#ifndef _WIN32
            + "- binbuild: " + string(K_BUILD) + '\n'
            + "- os-uname: " + FN::output("uname -srvm")
            + "- tracelog: " + '\n';
          void *k[69];
          size_t jumps = backtrace(k, 69);
          char **trace = backtrace_symbols(k, jumps);
          size_t i;
          for (i = 0; i < jumps; i++)
            tracelog += string(trace[i]) + '\n';
          free(trace)
#endif
          ;
          tracelog += '\n' + string(BRED) + "Yikes!" + string(RRED)
            + '\n' + "please copy and paste the error above into a new github issue (noworry for duplicates)."
            + '\n' + "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
            + '\n' + '\n';
        }
        halt(EXIT_FAILURE);
      };
  } ending;
  class Klass {
    protected:
      virtual void load() {};
      virtual void waitData() {};
      virtual void waitTime() {};
      virtual void waitUser() {};
      virtual void run() {};
      virtual void end() {};
    public:
      inline void wait() {
        load();
        waitData();
        waitTime();
        waitUser();
        run();
        endingFn.push_back([&](){
          end();
        });
        if (gw->randId) events->start();
      };
  };
}

#endif
#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  static struct Screen {
    Screen() {
      cout << BGREEN << "K"
           << RGREEN << " build " << K_BUILD << ' ' << K_STAMP
           << ".\n";
      string changes;
      int commits = -1;
      if (mCommand::git()) {
        mCommand::fetch();
        changes = mCommand::changelog();
        commits = count(changes.begin(), changes.end(), '\n');
      }
      cout << BGREEN << K_0_DAY << RGREEN << ' ' << (commits == -1
        ? "(zip install)"
        : (commits
          ? '-' + to_string(commits) + "commit" + (commits == 1?"":"s") + '.'
          : "(0day)"
        )
      )
#ifndef NDEBUG
      << " with DEBUG MODE enabled"
#endif
      << ".\n" << RYELLOW << changes << RRESET;
    };
    virtual void config() = 0;
    virtual void pressme(const mHotkey&, function<void()>) = 0;
    virtual void printme(mToScreen *const) = 0;
    virtual int error(string, string, bool = false) = 0;
    virtual void waitForUser() = 0;
    virtual string stamp() = 0;
    virtual void logWar(string, string, string = " Warrrrning: ") = 0;
    virtual void logUI(const string&) = 0;
    virtual void logUIsess(int, string) = 0;
    virtual void log(const string&, const string&, const string& = "") = 0;
#define PRETTY_DEBUG if (args.debugEvents) screen->log("DEBUG EV", __PRETTY_FUNCTION__);
#define DEBUG(x)     if (args.debugQuotes) screen->log("DEBUG QE", x)
#define DEBUQ(x, b, a, q) DEBUG("quote " x " " + to_string((int)b) + ":" + to_string((int)a) + " " + ((json)q).dump())
    virtual void end() = 0;
  } *screen = nullptr;

  static struct Events {
    virtual void deferred(const function<void()>&) = 0;
  } *events = nullptr;

  static struct Sqlite {
    virtual void backup(mFromDb *const) = 0;
  } *sqlite = nullptr;

  static struct Client {
    uWS::Hub* socket = nullptr;
    virtual void timer_Xs() = 0;
    virtual void welcome(mToClient&) = 0;
    virtual void clickme(mFromClient&, function<void(const json&)> = [](const json &butterfly) {}) = 0;
#define KISS , [&](const json &butterfly)
  } *client = nullptr;

  static struct Engine {
    virtual void timer_1s() = 0;
    virtual void calcQuote() = 0;
    virtual void calcQuoteAfterSavedParams() = 0;
  } *engine = nullptr;

  struct GwExchangeData {
    bool async = false;
    function<void(const mOrder&)>        write_mOrder;
    function<void(const mTrade&)>        write_mTrade;
    function<void(const mLevels&)>       write_mLevels;
    function<void(const mWallets&)>      write_mWallets;
    function<void(const mConnectivity&)> write_mConnectivity;
#define RAWDATA_ENTRY_POINT(mData, read) write_##mData = [&](const mData &rawdata) read
    bool waitForData() {
      return (async
        ? false
        : waitFor(replyOrders, write_mOrder)
          | waitFor(replyLevels, write_mLevels)
          | waitFor(replyTrades, write_mTrade)
      ) | waitFor(replyWallets, write_mWallets)
        | waitFor(replyCancelAll, write_mOrder);
    };
    function<bool()> askForWallet = [&]() { return !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); })); };
    function<bool()> askForLevels = [&]() { return askFor(replyLevels, [&]() { return sync_levels(); }); };
    function<bool()> askForTrades = [&]() { return askFor(replyTrades, [&]() { return sync_trades(); }); };
    function<bool()> askForOrders = [&]() { return askFor(replyOrders, [&]() { return sync_orders(); }); };
    function<bool()> askForCancelAll = [&]() { return askFor(replyCancelAll, [&]() { return sync_cancelAll(); }); };
//BO non-free gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/virtual bool ready() = 0;                                                // wait for exchange and maybe set async = true
/**/function<void(mRandId, string)> replace;                                 // call         async orders data from exchange
/**/virtual void place(mRandId, mSide, string, string, mOrderType, mTimeInForce, bool) = 0, // async orders like above/below
/**/             cancel(mRandId, mRandId) = 0,                               // call         async orders data from exchange
/**/             close() = 0;                                                // disconnect but without waiting for reconnect
/**/virtual vector<mOrder>   sync_cancelAll() = 0;                           // call and read sync orders data from exchange
/**/protected:
/**/  virtual bool            async_wallet() { return false; };              // call         async wallet data from exchange
/**/  virtual vector<mWallets> sync_wallet() { return {}; };                 // call and read sync wallet data from exchange
/**/  virtual vector<mLevels>  sync_levels()  { return {}; };                // call and read sync levels data from exchange
/**/  virtual vector<mTrade>   sync_trades()  { return {}; };                // call and read sync trades data from exchange
/**/  virtual vector<mOrder>   sync_orders()  { return {}; };                // call and read sync orders data from exchange
//EO non-free gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      future<vector<mWallets>> replyWallets;
      future<vector<mLevels>> replyLevels;
      future<vector<mTrade>> replyTrades;
      future<vector<mOrder>> replyOrders;
      future<vector<mOrder>> replyCancelAll;
      template<typename mData, typename syncFn> bool askFor(
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
      template<typename mData> unsigned int waitFor(
              future<vector<mData>>        &reply,
        const function<void(const mData&)> &write
      ) {
        bool waiting = reply.valid();
        if (waiting and reply.wait_for(chrono::nanoseconds(0))==future_status::ready) {
          for (mData &it : reply.get()) write(it);
          waiting = false;
        }
        return waiting;
      };
  };

  struct GwExchange: public GwExchangeData,
                     public mToScreen {
    uWS::Hub *socket  = nullptr;
    mNotepad notepad;
    mMonitor monitor;
    mSemaphore semaphore;
    unsigned int countdown = 0;
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
    bool refreshWallet = false;
    mRandId (*randId)() = nullptr;
    void connect() {
      socket->connect(ws, nullptr, {}, 5e+3, &socket->getDefaultGroup<uWS::CLIENT>());
    };
    void run() {
      monitor.fromGw(exchange, mPair(base, quote), &minTick);
      if (async) countdown = 1;
      socket->run();
    };
    void clear() {
      if (args.dustybot)
        print("GW " + name, "--dustybot is enabled, remember to cancel manually any open order.");
      else if (write_mOrder) {
        print("GW " + name, "Attempting to cancel all open orders, please wait.");
        for (mOrder &it : sync_cancelAll()) write_mOrder(it);
        print("GW " + name, "cancel all open orders OK");
      }
    };
    protected:
      void reconnect(const string &reason) {
        countdown = 7;
        print("GW " + name, "WS " + reason + ", reconnecting in " + to_string(countdown) + "s.");
      };
      void log(const string &reason) {
        const string prefix = string(
          reason.find(">>>") != reason.find("<<<")
            ? "DEBUG" : "GW"
        ) + ' ' + name;
        if (reason.find("Error") == string::npos)
          print(prefix, reason);
        else warn(prefix, reason);
      };
  };

  static struct Gw: public GwExchange {
    mWalletPosition wallet;
      mMarketLevels levels;
            mBroker broker;
    void placeOrder(
      const mSide        &side    ,
      const mPrice       &price   ,
      const mAmount      &qty     ,
      const mOrderType   &type    ,
      const mTimeInForce &tif     ,
      const bool         &isPong  ,
      const bool         &postOnly
    ) {
      mOrder *const o = broker.upsert(mOrder(randId(), side, qty, type, isPong, price, tif, mStatus::New, postOnly), true);
      place(o->orderId, o->side, str8(o->price), str8(o->quantity), o->type, o->timeInForce, o->preferPostOnly);
    };
    void replaceOrder(mOrder *const toReplace, const mPrice &price, const bool &isPong) {
      if (broker.replace(toReplace, price, isPong))
        replace(toReplace->exchangeId, str8(toReplace->price));
    };
    void cancelOrder(mOrder *const toCancel) {
      if (broker.cancel(toCancel))
        cancel(toCancel->orderId, toCancel->exchangeId);
    };
//BO non-free gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on arguments).
/**/static Gw* config(mCoinId, mCoinId, string, int, string, string, string, string, string, string, int, int); // set args..
//EO non-free gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on arguments).
  } *gw = nullptr;

  static string tracelog;
  static vector<function<void()>> happyEndingFn, endingFn = { []() {
    screen->end();
    cout << '\n' << screen->stamp() << tracelog;
  } };
  static struct Ending {
    Ending (/* KMxTWEpb9ig */) {
      tracelog = "- roll-out: " + to_string(Tstamp) + '\n';
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
        EXIT(code);
      };
      static void quit(const int sig) {
        tracelog = "Excellent decision! "
          + mREST::xfer("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
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
        if (mCommand::deprecated())
          tracelog += string("(deprecated K version found).") + '\n'
            + '\n' + string(BYELLOW) + "Hint!" + string(RYELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + mCommand::changelog()
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        else {
          tracelog += string("(Three-Headed Monkey found):") + '\n'
            + "- exchange: " + args.exchange + '\n'
            + "- currency: " + args.currency + '\n'
            + rollout
            + "- lastbeat: " + to_string(Tstamp) + '\n'
            + "- binbuild: " + string(K_BUILD) + '\n'
#ifndef _WIN32
            + "- os-uname: " + mCommand::uname()
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
      virtual void waitWebAdmin() {};
      virtual void waitSysAdmin() {};
      virtual void run() {};
      virtual void end() {};
    public:
      void wait() {
        load();
        waitData();
        if (!args.headless)
          waitWebAdmin();
        waitSysAdmin();
        waitTime();
        run();
        endingFn.push_back([&](){
          end();
        });
        if (gw->ready()) gw->run();
      };
  };
}

#endif
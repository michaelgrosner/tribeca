//! \file
//! \brief Exchange API integrations.

namespace ₿ {
  enum class Connectivity: unsigned int { Disconnected, Connected };
  enum class       Status: unsigned int { Waiting, Working, Terminated };
  enum class         Side: unsigned int { Bid, Ask };
  enum class  TimeInForce: unsigned int { GTC, IOC, FOK };
  enum class    OrderType: unsigned int { Limit, Market };
  enum class       Future: unsigned int { Spot, Inverse, Linear };

  struct Level {
     Price price = 0;
    Amount size  = 0;
  };
  static void to_json(json &j, const Level &k) {
    j = {
      {"price", k.price}
    };
    if (k.size) j["size"] = k.size;
  };
  struct Levels {
    vector<Level> bids,
                  asks;
  };
  static void __attribute__ ((unused)) to_json(json &j, const Levels &k) {
    j = {
      {"bids", k.bids},
      {"asks", k.asks}
    };
  };

  struct Wallet {
    string currency = "";
    Amount amount   = 0,
           held     = 0,
           total    = 0,
           value    = 0;
    double profit   = 0;
    static void reset(const Amount &a, const Amount &h, Wallet *const wallet) {
      wallet->total = (wallet->amount = a)
                    + (wallet->held   = h);
    };
  };
  static void to_json(json &j, const Wallet &k) {
    j = {
      {"amount", k.amount},
      {  "held", k.held  },
      { "value", k.value },
      {"profit", k.profit}
    };
  };
  struct Wallets {
    Wallet base,
           quote;
  };
  static void __attribute__ ((unused)) to_json(json &j, const Wallets &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };

  struct Trade {
      Side side     = (Side)0;
     Price price    = 0;
    Amount quantity = 0;
     Clock time     = 0;
  };
  static void __attribute__ ((unused)) to_json(json &j, const Trade &k) {
    j = {
      {    "side", k.side    },
      {   "price", k.price   },
      {"quantity", k.quantity},
      {    "time", k.time    }
    };
  };

  struct Order {
           Side side        = (Side)0;
          Price price       = 0;
         Amount quantity    = 0;
          Clock time        = 0;
           bool isPong      = false;
         string orderId     = "",
                exchangeId  = "";
         Status status      = (Status)0;
         Amount filled      = 0;
      OrderType type        = (OrderType)0;
    TimeInForce timeInForce = (TimeInForce)0;
           bool postOnly    = true;
          Clock latency     = 0;
    static void update(const Order &raw, Order *const order) {
      if (!order) return;
      if (Status::Working == (     order->status     = raw.status
      ) and !order->latency)       order->latency    = raw.time - order->time;
      order->time = raw.time;
      if (!raw.exchangeId.empty()) order->exchangeId = raw.exchangeId;
      if (raw.price)               order->price      = raw.price;
      if (raw.quantity)            order->quantity   = raw.quantity;
    };
    static bool replace(const Price &price, const bool &isPong, Order *const order) {
      if (!order
        or order->exchangeId.empty()
      ) return false;
      order->price  = price;
      order->isPong = isPong;
      order->time   = Tstamp;
      return true;
    };
    static bool cancel(Order *const order) {
      if (!order
        or order->exchangeId.empty()
        or order->status == Status::Waiting
      ) return false;
      order->status = Status::Waiting;
      order->time   = Tstamp;
      return true;
    };
  };
  static void __attribute__ ((unused)) to_json(json &j, const Order &k) {
    j = {
      {    "orderId", k.orderId    },
      { "exchangeId", k.exchangeId },
      {       "side", k.side       },
      {   "quantity", k.quantity   },
      {       "type", k.type       },
      {     "isPong", k.isPong     },
      {      "price", k.price      },
      {"timeInForce", k.timeInForce},
      {     "status", k.status     },
      {   "postOnly", k.postOnly   },
      {       "time", k.time       },
      {    "latency", k.latency    }
    };
  };
  static void __attribute__ ((unused)) from_json(const json &j, Order &k) {
    k.price           = j.value("price", 0.0);
    k.quantity        = j.value("quantity", 0.0);
    k.side            = j.value("side", "") == "Bid"
                          ? Side::Bid
                          : Side::Ask;
    k.type            = j.value("type", "") == "Limit"
                          ? OrderType::Limit
                          : OrderType::Market;
    k.timeInForce     = j.value("timeInForce", "") == "GTC"
                          ? TimeInForce::GTC
                          : (j.value("timeInForce", "") == "FOK"
                            ? TimeInForce::FOK
                            : TimeInForce::IOC);
    k.isPong          = false;
    k.postOnly        = false;
  };

  class GwExchangeData {
    public_friend:
      using DataEvent = variant<
        function<void(const Connectivity&)>,
        function<void(const Wallets&)>,
        function<void(const Levels&)>,
        function<void(const Order&)>,
        function<void(const Trade&)>
      >;
    public:
      curl_socket_t loopfd = 0;
      struct {
        Decimal funds,
                price,
                amount,
                percent;
      } decimal;
      bool askForFees      = false,
           askForReplace   = false,
           askForCancelAll = false;
      string (*randId)() = nullptr;
      virtual void ask_for_data(const unsigned int &tick) = 0;
      virtual void wait_for_data(Loop *const loop) = 0;
      void data(const DataEvent &ev) {
        if (holds_alternative<function<void(const Connectivity&)>>(ev))
          async.connectivity.write = get<function<void(const Connectivity&)>>(ev);
        else if (holds_alternative<function<void(const Wallets&)>>(ev))
          async.wallets.write      = get<function<void(const Wallets&)>>(ev);
        else if (holds_alternative<function<void(const Levels&)>>(ev))
          async.levels.write       = get<function<void(const Levels&)>>(ev);
        else if (holds_alternative<function<void(const Order&)>>(ev))
          async.orders.write       =
          async.cancelAll.write    = get<function<void(const Order&)>>(ev);
        else if (holds_alternative<function<void(const Trade&)>>(ev))
          async.trades.write       = get<function<void(const Trade&)>>(ev);
      };
      void place(const Order *const order) {
        place(
          order->orderId,
          order->side,
          decimal.price.str(order->price),
          decimal.amount.str(order->quantity),
          order->type,
          order->timeInForce,
          order->postOnly
        );
      };
      void replace(const Order *const order) {
        replace(
          order->exchangeId,
          decimal.price.str(order->price)
        );
      };
      void cancel(const Order *const order) {
        cancel(
          order->orderId,
          order->exchangeId
        );
      };
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual void replace(string, string) {};                               // call         async orders data from exchange
/**/  virtual void   place(string, Side, string, string, OrderType, TimeInForce, bool) = 0; // async orders like above/below
/**/  virtual void  cancel(string, string) = 0;                              // call         async orders data from exchange
/**/protected:
/**/  virtual             bool async_wallet()    { return false; };          // call         async wallet data from exchange
/**/  virtual             bool async_cancelAll() { return false; };          // call         async orders data from exchange
/**/  virtual vector<Wallets>   sync_wallet()    { return {}; };             // call and read sync wallet data from exchange
/**/  virtual  vector<Levels>   sync_levels()    { return {}; };             // call and read sync levels data from exchange
/**/  virtual   vector<Trade>   sync_trades()    { return {}; };             // call and read sync trades data from exchange
/**/  virtual   vector<Order>   sync_orders()    { return {}; };             // call and read sync orders data from exchange
/**/  virtual   vector<Order>   sync_cancelAll() { return {}; };             // call and read sync orders data from exchange
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      struct {
        Loop::Async::Event<Wallets>      wallets;
        Loop::Async::Event<Levels>       levels;
        Loop::Async::Event<Trade>        trades;
        Loop::Async::Event<Order>        orders,
                                         cancelAll;
        Loop::Async::Event<Connectivity> connectivity;
      } async;
      void online(const Connectivity &connectivity = Connectivity::Connected) {
        async.connectivity.try_write(connectivity);
        if (!(bool)connectivity)
          async.levels.try_write({});
      };
      void wait_for_never_async_data(Loop *const loop) {
        async.wallets.wait_for(loop,   [&]() { return sync_wallet(); });
        async.cancelAll.wait_for(loop, [&]() { return sync_cancelAll(); });
      };
      void wait_for_sync_data(Loop *const loop) {
        async.orders.wait_for(loop,    [&]() { return sync_orders(); });
        wait_for_never_async_data(loop);
        async.levels.wait_for(loop,    [&]() { return sync_levels(); });
        async.trades.wait_for(loop,    [&]() { return sync_trades(); });
      };
      void ask_for_never_async_data(const unsigned int &tick) {
        if (((askForFees and !(askForFees = false))
          or !(tick % 15))
          and !async_wallet())    async.wallets.ask_for();
        if ((askForCancelAll
          and !(tick % 300))
          and !async_cancelAll()) async.cancelAll.ask_for();
      };
      void ask_for_sync_data(const unsigned int &tick) {
        if (!(tick % 2))          async.orders.ask_for();
        ask_for_never_async_data(tick);
        if (!(tick % 3))          async.levels.ask_for();
        if (!(tick % 60))         async.trades.ask_for();
      };
  };

  class GwExchange: public GwExchangeData {
    public:
      using Report = vector<pair<string, string>>;
      string exchange,   apikey,    secret, pass,
             base,       quote,     symbol,
             http,       ws,        fix,
             webMarket,  webOrders, unlock;
       Price tickPrice = 0;
      Amount tickSize  = 0,
             minSize   = 0,
             makeFee   = 0,
             takeFee   = 0;
      size_t maxLevel  = 0;
      double leverage  = 0;
      Future margin    = (Future)0;
         int debug     = 0;
      Connectivity adminAgreement = Connectivity::Disconnected;
      virtual void disconnect() {};
      virtual bool connected() const { return true; };
      virtual json handshake() = 0;
      json handshake(const bool &nocache) {
        json reply;
        const string cache = "/var/lib/K/cache/handshake"
              + ('.' + exchange)
              +  '.' + base
              +  '.' + quote
              +  '.' + "json";
        fstream file;
        struct stat st;
        if (!nocache
          and access(cache.data(), R_OK) != -1
          and !stat(cache.data(), &st)
          and Tstamp - 25200e+3 < st.st_mtime * 1e+3
        ) {
          file.open(cache, fstream::in);
          reply = json::parse(file);
        } else
          reply = handshake();
        base      = reply.value("base",      base);
        quote     = reply.value("quote",     quote);
        symbol    = reply.value("symbol",    symbol);
        margin    = reply.value("margin",    margin);
        webMarket = reply.value("webMarket", webMarket);
        webOrders = reply.value("webOrders", webOrders);
        tickPrice = reply.value("tickPrice", 0.0);
        tickSize  = reply.value("tickSize",  0.0);
        if (!minSize) minSize = reply.value("minSize", 0.0);
        if (!makeFee) makeFee = reply.value("makeFee", 0.0);
        if (!takeFee) takeFee = reply.value("takeFee", 0.0);
        decimal.funds.precision(1e-8);
        decimal.price.precision(tickPrice);
        decimal.amount.precision(tickSize);
        decimal.percent.precision(1e-2);
        if (!file.is_open()
          and tickPrice and tickSize and minSize
          and !base.empty() and !quote.empty()
        ) {
          file.open(cache, fstream::out | fstream::trunc);
          file << reply.dump();
        }
        if (file.is_open()) file.close();
        return reply.value("reply", json::object());
      };
      void purge(const bool &dustybot) {
        if (dustybot)
          print("--dustybot is enabled, remember to cancel manually any open order.");
        else {
          print("Attempting to cancel all open orders, please wait.");
          if (!async_cancelAll()) sync_cancelAll();
          print("cancel all open orders OK");
        }
      };
      void end() {
        online(Connectivity::Disconnected);
        disconnect();
      };
      void report(Report notes, const bool &nocache) {
        if (exchange == "NULL") print("all data is random");
        for (auto it : (Report){
          {"symbols", (margin == Future::Linear
                        ? symbol             + " (" + decimal.funds.str(decimal.funds.step)
                        : base + "/" + quote + " (" + decimal.amount.str(tickSize)
                      ) + "/"
                        + decimal.price.str(tickPrice) + ")"                              },
          {"minSize", decimal.amount.str(minSize) + " " + (
                        margin == Future::Spot
                          ? base
                          : "Contract" + string(minSize == 1 ? 0 : 1, 's')
                      )                                                                   },
          {"makeFee", decimal.percent.str(makeFee * 1e+2) + "%"
                        + (makeFee ? "" : " (please use --maker-fee argument!)")          },
          {"takeFee", decimal.percent.str(takeFee * 1e+2) + "%"
                        + (takeFee ? "" : " (please use --taker-fee argument!)")          }
        }) notes.push_back(it);
        string note = "handshake:";
        for (auto &it : notes)
          if (!it.second.empty())
            note += "\n- " + it.first + ": " + it.second;
        print((nocache ? "" : "cached ") + note);
      };
      void latency(const string &reason, const function<void()> &fn) {
        print("latency check", "start");
        const Clock Tstart = Tstamp;
        fn();
        const Clock Tstop  = Tstamp;
        print("latency check", "stop");
        const unsigned int Tdiff = Tstop - Tstart;
        print(reason + " took", to_string(Tdiff) + "ms of your time");
        string result = "This result is ";
        if      (Tdiff < 2e+2) result += "very good; most traders don't enjoy such speed!";
        else if (Tdiff < 5e+2) result += "good; most traders get the same result";
        else if (Tdiff < 7e+2) result += "a bit bad; most traders get better results";
        else if (Tdiff < 1e+3) result += "bad; consider moving to another server/network";
        else                   result += "very bad; move to another server/network";
        print(result);
      };
      void disclaimer() const {
        if (unlock.empty()) return;
        print("was slowdown 7 seconds (--free-version argument was implicitly set):"
          "\n" "\n" "Your apikey: " + apikey +
          "\n" "\n" "To unlock it anonymously and to collaborate with"
          "\n"      "the development, make an acceptable Pull Request"
          "\n"      "on github.. or send 0.01210000 BTC (or more) to:"
          "\n" "\n" "  " + unlock +
          "\n" "\n" "Before restart just wait for 0 confirmations at:"
          "\n"      "https://live.blockcypher.com/btc/address/" + unlock +
          "\n" "\n" OBLIGATORY_analpaper_SOFTWARE_LICENSE
          "\n" "\n" "                     Signed-off-by: Carles Tubio"
          "\n"      "see: github.com/ctubio/Krypto-trading-bot#unlock"
          "\n"      "or just use --free-version to hide this message"
        );
      };
      function<void(const string&, const string&, const string&)> printer;
    protected:
      void print(const string &reason, const string &highlight = "") const {
        if (printer) printer(
          string(reason.find(">>>") != reason.find("<<<")
            ? "DEBUG "
            : "GW "
          ) + exchange,
          reason,
          highlight
        );
      };
      void reduce(Levels &levels) {
        if (maxLevel) {
          if (levels.bids.size() > maxLevel)
            levels.bids.erase(levels.bids.begin() + maxLevel, levels.bids.end());
          if (levels.asks.size() > maxLevel)
            levels.asks.erase(levels.asks.begin() + maxLevel, levels.asks.end());
        }
      };
  };

  class Gw: public GwExchange {
    public:
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on argument).
/**/  static Gw* new_Gw(const string&); // may return too a nullptr instead of a child gateway class, if string is unknown..
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on argument).
  };

  class GwApiREST: public Gw {
    public:
      void ask_for_data(const unsigned int &tick) override {
        ask_for_sync_data(tick);
      };
      void wait_for_data(Loop *const loop) override {
        online();
        wait_for_sync_data(loop);
      };
  };
  class GwApiWs: public Gw,
                 public Curl::WebSocket {
    private:
       unsigned int countdown    = 1;
               bool subscription = false;
    public:
      bool connected() const override {
        return WebSocket::connected();
      };
      void ask_for_data(const unsigned int &tick) override {
        if (countdown and !--countdown)
          connect();
        if (subscribed())
          ask_for_never_async_data(tick);
      };
      void wait_for_data(Loop *const loop) override {
        wait_for_never_async_data(loop);
      };
    protected:
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual void subscribe()   = 0;                                         // send subcription messages to remote server.
/**/  virtual void consume(json) = 0;                                         // read message one by one from remote server.
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      virtual void connect() {
        CURLcode rc;
        if (CURLE_OK == (rc = WebSocket::connect(ws)))
          WebSocket::start(GwExchangeData::loopfd, [&]() {
            wait_for_async_data();
          });
        else reconnect(string("CURL connect Error: ") + curl_easy_strerror(rc));
      };
      void emit(const string &msg) {
        CURLcode rc;
        if (CURLE_OK != (rc = WebSocket::emit(msg, 0x01)))
          print(string("CURL send Error: ") + curl_easy_strerror(rc));
      };
      void disconnect() override {
        WebSocket::emit("", 0x08);
        WebSocket::cleanup();
      };
      void reconnect(const string &reason) {
        disconnect();
        countdown = 7;
        print("WS " + reason + ", reconnecting in " + to_string(countdown) + "s.");
      };
      bool accept_msg(const string &msg) {
        const bool next = !msg.empty();
        if (next) {
          if (json::accept(msg))
            consume(json::parse(msg));
          else print("CURL Error: Unsupported data format");
        }
        return next;
      };
      bool subscribed() {
        if (subscription != connected()) {
          subscription = !subscription;
          if (subscription) subscribe();
          else {
            online(Connectivity::Disconnected);
            reconnect("Disconnected");
          };
        }
        return subscription;
      };
    private:
      void wait_for_async_data() {
        CURLcode rc;
        if (CURLE_OK != (rc = WebSocket::send_recv()))
          print(string("CURL recv Error: ") + curl_easy_strerror(rc));
        while (accept_msg(WebSocket::unframe()));
      };
  };
  class GwApiFix: public GwApiWs,
                  public Curl::FixSocket {
    protected:
      string target;
    public:
      GwApiFix()
        : FixSocket(apikey, target)
      {};
      bool connected() const override {
        return WebSocket::connected()
           and FixSocket::connected();
      };
    protected:
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual string logon() = 0;                                                                   // return logon message.
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      void connect() override {
        GwApiWs::connect();
        if (WebSocket::connected()) {
          CURLcode rc;
          if (CURLE_OK == (rc = FixSocket::connect(fix, logon()))) {
            FixSocket::start(GwExchangeData::loopfd, [&]() {
              wait_for_async_data();
            });
            print("FIX success Logon, streaming orders");
          } else reconnect(string("CURL connect FIX Error: ") + curl_easy_strerror(rc));
        }
      };
      void disconnect() override {
        if (FixSocket::connected()) print("FIX Logout");
        FixSocket::emit("", "5");
        FixSocket::cleanup();
        GwApiWs::disconnect();
      };
      void beam(const string &msg, const string &type) {
        CURLcode rc;
        if (CURLE_OK != (rc = FixSocket::emit(msg, type)))
          print(string("CURL send FIX Error: ") + curl_easy_strerror(rc));
      };
    private:
      void wait_for_async_data() {
        CURLcode rc;
        if (CURLE_OK != (rc = FixSocket::send_recv()))
          print(string("CURL recv FIX Error: ") + curl_easy_strerror(rc));
        while (accept_msg(FixSocket::unframe()));
      };
  };

  class GwNull: public GwApiREST {
    public:
      GwNull()
      {
        randId = Random::uuid36Id;
      };
    public:
      json handshake() override {
        return {
          {     "base", base     },
          {    "quote", quote    },
          {   "margin", margin   },
          {"webMarket", webMarket},
          {"webOrders", webOrders},
          {"tickPrice", 1e-2     },
          { "tickSize", 1e-2     },
          {  "minSize", 1e-2     },
          {    "reply", nullptr  }
        };
      };
  };
  class GwBitmex: public GwApiWs {
    public:
      GwBitmex()
      {
        http   = "https://www.bitmex.com/api/v1";
        ws     = "wss://www.bitmex.com/realtime";
        randId = Random::uuid36Id;
        askForReplace = true;
        webMarket = "https://www.bitmex.com/app/trade/";
        webOrders = "https://www.bitmex.com/app/orderHistory";
      };
      json handshake() override {
        symbol = base + quote;
        webMarket += base + quote;
        json reply = Curl::Web::xfer(http + "/instrument?symbol=" + symbol);
        if (reply.is_array()) {
          if (reply.empty())
            reply = {
              {"error", symbol + " is not a valid symbol"}
            };
          else
            for (const json &it : reply) {
              reply = it;
              break;
            }
        }
        return {
          {     "base", "XBT"                          },
          {    "quote", quote                          },
          {   "symbol", symbol                         },
          {   "margin", reply.value("isInverse", false)
                          ? Future::Inverse
                          : Future::Linear             },
          {"webMarket", webMarket                      },
          {"webOrders", webOrders                      },
          {"tickPrice", reply.value("tickSize", 0.0)   },
          { "tickSize", reply.value("lotSize", 0.0)    },
          {  "minSize", reply.value("lotSize", 0.0)    },
          {  "makeFee", reply.value("makerFee", 0.0)   },
          {  "takeFee", reply.value("takerFee", 0.0)   },
          {    "reply", reply                          }
        };
      };
    protected:
      static json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &post, const string &crud) {
        return Curl::Web::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("api-expires: "   + h1).data());
          h_ = curl_slist_append(h_, ("api-key: "       + h2).data());
          h_ = curl_slist_append(h_, ("api-signature: " + h3).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwHitBtc: public GwApiWs {
    public:
      GwHitBtc()
      {
        http   = "https://api.hitbtc.com/api/2";
        ws     = "wss://api.hitbtc.com/api/2/ws";
        randId = Random::uuid32Id;
        webMarket = "https://hitbtc.com/exchange/";
        webOrders = "https://hitbtc.com/reports/orders";
      };
      json handshake() override {
        symbol = base + quote;
        webMarket += base + "-to-" + quote;
        const json reply = Curl::Web::xfer(http + "/public/symbol/" + symbol);
        return {
          {     "base", base == "USDT" ? "USD" : base                 },
          {    "quote", quote == "USDT" ? "USD" : quote               },
          {   "symbol", symbol                                        },
          {"webMarket", webMarket                                     },
          {"webOrders", webOrders                                     },
          {   "margin", margin                                        },
          {"tickPrice", stod(reply.value("tickSize", "0"))            },
          { "tickSize", stod(reply.value("quantityIncrement", "0"))   },
          {  "minSize", stod(reply.value("quantityIncrement", "0"))   },
          {  "makeFee", stod(reply.value("provideLiquidityRate", "0"))},
          {  "takeFee", stod(reply.value("takeLiquidityRate", "0"))   },
          {    "reply", reply                                         }
        };
      };
    protected:
      static json xfer(const string &url, const string &auth, const string &post) {
        return Curl::Web::request(url, [&](CURL *curl) {
          curl_easy_setopt(curl, CURLOPT_USERPWD, auth.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwBequant: virtual public GwHitBtc {
    public:
      GwBequant()
      {
        http = "https://api.bequant.io/api/2";
        ws   = "wss://api.bequant.io/api/2/ws";
        webMarket = "https://bequant.io/exchange/";
        webOrders = "https://bequant.io/reports/orders";
      };
  };
  class GwCoinbase: public GwApiFix {
    public:
      GwCoinbase()
      {
        http   = "https://api.pro.coinbase.com";
        ws     = "wss://ws-feed.pro.coinbase.com";
        fix    = "fix.pro.coinbase.com:4198";
        target = "Coinbase";
        randId = Random::uuid36Id;
        webMarket = "https://pro.coinbase.com/trade/";
        webOrders = "https://pro.coinbase.com/orders/";
      };
      json handshake() override {
        symbol = base + "-" + quote;
        webMarket += base + quote;
        webOrders += base + quote;
        const json reply = Curl::Web::xfer(http + "/products/" + symbol);
        return {
          {     "base", base                                     },
          {    "quote", quote                                    },
          {   "symbol", symbol                                   },
          {   "margin", margin                                   },
          {"webMarket", webMarket                                },
          {"webOrders", webOrders                                },
          {"tickPrice", stod(reply.value("quote_increment", "0"))},
          { "tickSize", stod(reply.value("base_increment", "0")) },
          {  "minSize", stod(reply.value("base_min_size", "0"))  },
          {    "reply", reply                                    }
        };
      };
    protected:
      static json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &h4) {
        return Curl::Web::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("CB-ACCESS-KEY: "        + h1).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-SIGN: "       + h2).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-TIMESTAMP: "  + h3).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-PASSPHRASE: " + h4).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        });
      };
  };
  class GwBitfinex: public GwApiWs {
    public:
      GwBitfinex()
      {
        http   = "https://api.bitfinex.com/v1";
        ws     = "wss://api.bitfinex.com/ws/2";
        randId = Random::int45Id;
        askForReplace = true;
        webMarket = "https://www.bitfinex.com/trading/";
        webOrders = "https://www.bitfinex.com/reports/orders";
      };
      json handshake() override {
        symbol = base + quote;
        webMarket += symbol;
        const json reply1 = Curl::Web::xfer(http + "/pubticker/" + symbol);
        Price tickPrice = 0,
              minSize   = 0;
        if (reply1.find("last_price") != reply1.end()) {
          ostringstream price_;
          price_ << scientific << stod(reply1.value("last_price", "0"));
          string price = price_.str();
          for (string::iterator it=price.begin(); it!=price.end();)
            if (*it == '+' or *it == '-') break;
            else it = price.erase(it);
          istringstream iss("1e" + to_string(fmax(stod(price),-4)-4));
          iss >> tickPrice;
        }
        const json reply2 = Curl::Web::xfer(http + "/symbols_details");
        if (reply2.is_array())
          for (const json &it : reply2)
            if (it.find("pair") != it.end() and it.value("pair", "") == Text::strL(symbol)) {
              minSize = stod(it.value("minimum_order_size", "0"));
              break;
            }
        return {
          {     "base", base            },
          {    "quote", quote           },
          {   "symbol", symbol          },
          {   "margin", margin          },
          {"webMarket", webMarket       },
          {"webOrders", webOrders       },
          {"tickPrice", tickPrice       },
          { "tickSize", tickPrice < 1e-8
                         ? 1e-10
                         : 1e-8         },
          {  "minSize", minSize         },
          {    "reply", {reply1, reply2}}
        };
      };
    protected:
      static json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::Web::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("X-BFX-APIKEY: "    + h1).data());
          h_ = curl_slist_append(h_, ("X-BFX-PAYLOAD: "   + post).data());
          h_ = curl_slist_append(h_, ("X-BFX-SIGNATURE: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwEthfinex: virtual public GwBitfinex {
    public:
      GwEthfinex()
      {
        http = "https://api.ethfinex.com/v1";
        ws   = "wss://api.ethfinex.com/ws/2";
        webMarket = "https://www.ethfinex.com/trading/";
        webOrders = "https://www.ethfinex.com/reports/orders";
      };
  };
  class GwKraken: public GwApiREST {
    public:
      GwKraken()
      {
        http   = "https://api.kraken.com";
        randId = Random::int32Id;
        webMarket = "https://www.kraken.com/charts";
        webOrders = "https://www.kraken.com/u/trade";
      };
      json handshake() override {
        const json reply = Curl::Web::xfer(http + "/0/public/AssetPairs?pair=" + base + quote);
        Price tickPrice = 0,
              minSize   = 0;
        if (reply.find("result") != reply.end())
          for (json::const_iterator it = reply.at("result").cbegin(); it != reply.at("result").cend(); ++it)
            if (it.value().find("pair_decimals") != it.value().end()) {
              istringstream iss(
                "1e-" + to_string(it.value().value("pair_decimals", 0))
                + " 1e-" + to_string(it.value().value("lot_decimals", 0))
              );
              iss >> tickPrice >> minSize;
              base   = it.value().value("base", base);
              quote  = it.value().value("quote", quote);
              symbol = base + quote;
              break;
            }
        return {
          {     "base", base            },
          {    "quote", quote           },
          {   "symbol", symbol          },
          {   "margin", margin          },
          {"webMarket", webMarket       },
          {"webOrders", webOrders       },
          {"tickPrice", tickPrice       },
          { "tickSize", tickPrice < 1e-8
                         ? 1e-10
                         : 1e-8         },
          {  "minSize", minSize         },
          {    "reply", reply           }
        };
      };
    protected:
      static json xfer(const string &url, const string &h1, const string &h2, const string &post) {
        return Curl::Web::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("API-Key: "  + h1).data());
          h_ = curl_slist_append(h_, ("API-Sign: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwPoloniex: public GwApiREST {
    public:
      GwPoloniex()
      {
        http   = "https://poloniex.com";
        randId = Random::int45Id;
        webMarket = "https://poloniex.com/exchange";
        webOrders = "https://poloniex.com/tradeHistory";
      };
      json handshake() override {
        symbol = quote + "_" + base;
        const json reply = Curl::Web::xfer(http + "/public?command=returnTicker")
                             .value(symbol, json::object());
        const Price tickPrice = reply.empty()
                                  ? 0
                                  : 1e-8;
        return {
          {     "base", base            },
          {    "quote", quote           },
          {   "symbol", symbol          },
          {   "margin", margin          },
          {"webMarket", webMarket       },
          {"webOrders", webOrders       },
          {"tickPrice", tickPrice       },
          { "tickSize", tickPrice < 1e-8
                          ? 1e-10
                          : 1e-8        },
          {  "minSize", 1e-3            },
          {    "reply", reply           }
        };
      };
    protected:
      static json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::Web::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          h_ = curl_slist_append(h_, ("Key: "  + h1).data());
          h_ = curl_slist_append(h_, ("Sign: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
}

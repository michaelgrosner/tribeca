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

  struct Ticker {
    string base  = "";
    string quote = "";
     Price price = 0;
  };
  static void __attribute__ ((unused)) to_json(json &j, const Ticker &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote},
      {"price", k.price}
    };
  };

  struct Wallet {
    Amount amount   = 0,
           held     = 0;
    string currency = "";
    Amount total    = 0,
           value    = 0;
    double profit   = 0;
    Wallet &operator=(const Wallet &raw) {
      total = (amount = raw.amount)
            + (held   = raw.held);
      currency = raw.currency;
      return *this;
    };
  };
  static void __attribute__ ((unused)) to_json(json &j, const Wallet &k) {
    j = {
      {  "amount", k.amount  },
      {    "held", k.held    },
      {"currency", k.currency},
      {   "value", k.value   },
      {  "profit", k.profit  }
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
           bool manual      = false;
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
      {       "time", k.time       },
      {    "latency", k.latency    }
    };
  };
  static void __attribute__ ((unused)) from_json(const json &j, Order &k) {
    k.orderId     = j.value("orderId", "");
    k.price       = j.value("price", 0.0);
    k.quantity    = j.value("quantity", 0.0);
    k.time        = j.value("time", Tstamp);
    k.side        = j.value("side", k.side);
    k.type        = j.value("type", k.type);
    k.timeInForce = j.value("timeInForce", k.timeInForce);
    k.manual      = j.value("manual", false);
  };

  class GwExchangeData {
    public_friend:
      using DataEvent = variant<
        function<void(const Connectivity&)>,
        function<void(const Ticker&)>,
        function<void(const Wallet&)>,
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
      bool askForReplace = false;
      string (*randId)() = nullptr;
      virtual void ask_for_data(const unsigned int &tick) = 0;
      virtual void wait_for_data(Loop *const loop) = 0;
      void data(const DataEvent &ev) {
        if (holds_alternative           <function<void(const Connectivity&)>>(ev))
          async.connectivity.write = get<function<void(const Connectivity&)>>(ev);
        else if (holds_alternative      <function<void(const Ticker&)>>(ev))
          async.ticker.write       = get<function<void(const Ticker&)>>(ev);
        else if (holds_alternative      <function<void(const Wallet&)>>(ev))
          async.wallet.write       = get<function<void(const Wallet&)>>(ev);
        else if (holds_alternative      <function<void(const Levels&)>>(ev))
          async.levels.write       = get<function<void(const Levels&)>>(ev);
        else if (holds_alternative      <function<void(const Order&)>>(ev))
          async.orders.write       = get<function<void(const Order&)>>(ev);
        else if (holds_alternative      <function<void(const Trade&)>>(ev))
          async.trades.write       = get<function<void(const Trade&)>>(ev);
      };
      void place(const Order *const order) {
        place(
          order->orderId,
          order->side,
          decimal.price.str(order->price),
          decimal.amount.str(order->quantity),
          order->type,
          order->timeInForce
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
      void balance() {
        if (!async_wallet())
          async.wallet.ask_for();
      };
//BO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions below)...
/**/  virtual void   place(string, Side, string, string, OrderType, TimeInForce) = 0;// call async order  data from exchange.
/**/  virtual void replace(string, string) {};                                 // call price async order  data from exchange.
/**/  virtual void  cancel(string, string) = 0;                              // call by uuid async order  data from exchange.
/**/  virtual void  cancel() = 0;                                          // call by symbol async orders data from exchange.
/**/protected:
/**/  virtual           bool async_wallet() { return false; };         // call               async wallet data from exchange.
/**/  virtual vector<Wallet>  sync_wallet() { return {}; };          // call                  sync wallet data from exchange.
//EO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions above)...
      struct {
        Loop::Async::Event<Connectivity> connectivity;
        Loop::Async::Event<Ticker>       ticker;
        Loop::Async::Event<Wallet>       wallet;
        Loop::Async::Event<Levels>       levels;
        Loop::Async::Event<Order>        orders;
        Loop::Async::Event<Trade>        trades;
      } async;
      void online(const Connectivity &connectivity) {
        async.connectivity.try_write(connectivity);
        if (!(bool)connectivity)
          async.levels.try_write({});
      };
      void wait_for_never_async_data(Loop *const loop) {
        async.wallet.wait_for(loop, [&]() { return sync_wallet(); });
      };
      void ask_for_never_async_data(const unsigned int &tick) {
        if (async.wallet.write and !(tick % 15)) balance();
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
             minValue  = 0,
             makeFee   = 0,
             takeFee   = 0;
      size_t maxLevel  = 0;
      double leverage  = 0;
      Future margin    = (Future)0;
         int debug     = 0;
      Connectivity adminAgreement = Connectivity::Disconnected;
      json handshake(const bool &nocache) {
        json reply;
        const string cache = (K_HOME "/cache/handshake")
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
        minValue  = reply.value("minValue",  0.0);
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
      void end() {
        online(Connectivity::Disconnected);
        disconnect();
      };
      void report(Report notes, const bool &nocache) {
        for (const auto &it : (Report){
          {"symbols", (margin == Future::Linear
                        ? symbol             + " (" + decimal.funds.str(decimal.funds.step)
                        : base + "/" + quote + " (" + decimal.amount.str(tickSize)
                      ) + "/"
                        + decimal.price.str(tickPrice) + ")"                                  },
          {"minSize", decimal.amount.str(minSize) + " " + (
                        margin == Future::Spot
                          ? base
                          : "Contract" + string(minSize != 1, 's')
                      ) + (minValue ? " or " + decimal.price.str(minValue) + " " + quote : "")},
          {"makeFee", decimal.percent.str(makeFee * 1e+2) + "%"                               },
          {"takeFee", decimal.percent.str(takeFee * 1e+2) + "%"                               }
        }) notes.push_back(it);
        string note = "handshake:";
        for (const auto &it : notes)
          if (!it.second.empty())
            note += "\n- " + it.first + ": " + it.second;
        print((nocache ? "" : "cached ") + note);
      };
      string latency(const function<void()> &fn) {
        print("latency check", "start");
        const Clock Tstart = Tstamp;
        fn();
        const Clock Tstop  = Tstamp;
        print("latency check", "stop");
        const unsigned int Tdiff = Tstop - Tstart;
        print("HTTP read/write handshake took", to_string(Tdiff) + "ms of your time");
        string result = "This result is ";
        if      (Tdiff < 2e+2) result += "very good; most traders don't enjoy such speed!";
        else if (Tdiff < 5e+2) result += "good; most traders get the same result";
        else if (Tdiff < 7e+2) result += "a bit bad; most traders get better results";
        else if (Tdiff < 1e+3) result += "bad; consider moving to another server/network";
        else                   result += "very bad; move to another server/network";
        print(result);
        return "--latency of 1 HTTP call done (consider to repeat a few times this check)";
      };
      string pairs() const {
        string report;
        pairs(report);
        print("allows " + to_string(count(report.begin(), report.end(), '\n')) + " currency pairs");
        cout << report;
        return "--list done (to find a symbol use grep)";
      };
      void disclaimer() const {
        if (unlock.empty()) return;
        print("was slowdown 121 seconds (--free-version argument was implicitly set):"
          "\n" "\n" "Current apikey: " + apikey.substr(0, apikey.length() / 2)
                                       + string(apikey.length() / 2, '#') +
          "\n" "\n" "To unlock it anonymously and to collaborate with"
          "\n"      "the development, make an acceptable Pull Request"
          "\n"      "on github.. or send 0.01210000 BTC (or more) to:"
          "\n" "\n" "  " + unlock +
          "\n" "\n" "Before restart, wait for zero (0) confirmations:"
          "\n" "\n" "https://live.blockcypher.com/btc/address/" + unlock +
          "\n" "\n" OBLIGATORY_analpaper_SOFTWARE_LICENSE
          "\n" "\n" "                     Signed-off-by: Carles Tubio"
          "\n"      "see: github.com/ctubio/Krypto-trading-bot#unlock"
          "\n"      "or just use --free-version to hide this message"
        );
      };
      function<void(const string&, const string&, const string&)> printer;
    protected:
      virtual   void disconnect()         = 0;
      virtual   bool connected()    const = 0;
      virtual   json handshake()    const = 0;
      virtual   void pairs(string&) const = 0;
      virtual string nonce()        const = 0;
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
//BO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions below).
/**/  static Gw* new_Gw(const string&); // may return too a nullptr instead of a child gateway class, if string is unknown.
//EO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions above).
  };

  class GwApiWs: public Gw,
                 public Curl::WebSocket {
    private:
       unsigned int countdown    = 1;
               bool subscription = false;
    public:
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
//BO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions below).
/**/  virtual string subscribe() = 0;               // send subcription messages to remote server and return channel names.
/**/  virtual void consume(json) = 0;               // read message one by one from remote server and call async observers.
//EO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions above).
      bool connected() const override {
        return WebSocket::connected();
      };
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
      bool subscribed() {
        if (subscription != connected()) {
          subscription = !subscription;
          if (subscription) print("WS streaming [" + subscribe() + "]");
          else reconnect("Disconnected");
          online((Connectivity)subscription);
        }
        return subscription;
      };
      bool accept_msg(const string &msg) {
        const bool next = !msg.empty();
        if (next) {
          if (json::accept(msg))
            consume(json::parse(msg));
          else print("WS Error: Unsupported data format");
        }
        return next;
      };
    private:
      void wait_for_async_data() {
        CURLcode rc;
        if (CURLE_OK != (rc = WebSocket::send_recv()))
          print(string("CURL recv Error: ") + curl_easy_strerror(rc));
        while (accept_msg(WebSocket::unframe()));
      };
  };
  class GwApiWsWs: public GwApiWs,
                   public Curl::WebSocketTwin {
    protected:
      bool connected() const override {
        return GwApiWs::connected()
           and WebSocketTwin::connected();
      };
      void connect() override {
        GwApiWs::connect();
        if (GwApiWs::connected()) {
          CURLcode rc;
          if (CURLE_OK == (rc = WebSocketTwin::connect(twin(ws))))
            WebSocketTwin::start(GwExchangeData::loopfd, [&]() {
              wait_for_async_data();
            });
          else reconnect(string("CURL connect Error: ") + curl_easy_strerror(rc));
        }
      };
      void disconnect() override {
        WebSocketTwin::emit("", 0x08);
        WebSocketTwin::cleanup();
        GwApiWs::disconnect();
      };
      void emit(const string &msg) {
        GwApiWs::emit(msg);
      };
      void beam(const string &msg) {
        CURLcode rc;
        if (CURLE_OK != (rc = WebSocketTwin::emit(msg, 0x01)))
          print(string("CURL send Error: ") + curl_easy_strerror(rc));
      };
    private:
      void wait_for_async_data() {
        CURLcode rc;
        if (CURLE_OK != (rc = WebSocketTwin::send_recv()))
          print(string("CURL recv Error: ") + curl_easy_strerror(rc));
        while (accept_msg(WebSocketTwin::unframe()));
      };
  };
  class GwApiWsFix: public GwApiWs,
                    public Curl::FixSocket {
    public:
      GwApiWsFix(const string &t)
        : FixSocket(t, apikey)
      {};
    protected:
      bool connected() const override {
        return GwApiWs::connected()
           and FixSocket::connected();
      };
//BO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions below).
/**/  virtual string logon() = 0;                                                                  // return logon message.
//EO non-free Gw class member functions from lib build-*/lib/K-*.a (it just redefines all virtual gateway functions above).
      void connect() override {
        GwApiWs::connect();
        if (GwApiWs::connected()) {
          CURLcode rc;
          if (CURLE_OK == (rc = FixSocket::connect(fix, logon()))) {
            FixSocket::start(GwExchangeData::loopfd, [&]() {
              wait_for_async_data();
            });
            print("FIX Logon, streaming orders");
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

  class GwBinance: public GwApiWs {
    public:
      GwBinance()
      {
        http   = "https://api.binance.com";
        ws     = "wss://stream.binance.com:9443/ws";
        randId = Random::uuid36Id;
        webMarket = "https://www.binance.com/en/trade/";
        webOrders = "https://www.binance.com/en/my/orders/exchange/tradeorder";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/api/v3/exchangeInfo");
        if (!reply.is_object()
          or reply.find("symbols") == reply.end()
          or !reply.at("symbols").is_array()
          or reply.at("symbols").empty()
          or !reply.at("symbols").at(0).is_object()
          or reply.at("symbols").at(0).find("isSpotTradingAllowed") == reply.at("symbols").at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply.at("symbols"))
          if (it.value("isSpotTradingAllowed", false)
            and it.value("status", "") == "TRADING"
          ) report += it.value("baseAsset", "") + "/" + it.value("quoteAsset", "") + '\n';
      };
      json handshake() const override {
        json reply1 = Curl::Web::xfer(http + "/api/v3/exchangeInfo");
        if (reply1.find("symbols") != reply1.end() and reply1.at("symbols").is_array())
          for (const json &it : reply1.at("symbols"))
            if (it.value("symbol", "") == base + quote) {
              reply1 = it;
              if (reply1.find("filters") != reply1.end() and reply1.at("filters").is_array())
                for (const json &it_ : reply1.at("filters")) {
                  if (it_.value("filterType", "") == "PRICE_FILTER")
                    reply1["tickPrice"] = stod(it_.value("tickSize", "0"));
                  else if (it_.value("filterType", "") == "MIN_NOTIONAL")
                    reply1["minValue"] = stod(it_.value("minNotional", "0"));
                  else if (it_.value("filterType", "") == "LOT_SIZE") {
                    reply1["tickSize"] = stod(it_.value("stepSize", "0"));
                    reply1["minSize"] = stod(it_.value("minQty", "0"));
                  }
                }
              break;
            }
        const json reply2 = fees();
        return {
          {     "base", base                                      },
          {    "quote", quote                                     },
          {   "symbol", base + quote                              },
          {"webMarket", webMarket
                          + base + "_" + quote
                          + "?layout=pro"                         },
          {"webOrders", webOrders                                 },
          {"tickPrice", reply1.value("tickPrice", 0.0)            },
          { "tickSize", reply1.value("tickSize", 0.0)             },
          {  "minSize", reply1.value("minSize", 0.0)              },
          { "minValue", reply1.value("minValue", 0.0)             },
          {  "makeFee", stod(reply2.value("makerCommission", "0"))},
          {  "takeFee", stod(reply2.value("takerCommission", "0"))},
          {    "reply", {reply1, reply2}                          }
        };
      };
      json xfer(const string &url, const string &h1, const string &crud) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("X-MBX-APIKEY: " + h1).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
        });
      };
    private:
      json fees() const {
        const string crud = "GET",
                     path = "/sapi/v1/asset/tradeFee?",
                     post = "symbol="     + base + quote
                          + "&timestamp=" + nonce(),
                     sign = "&signature=" + Text::HMAC256(post, secret);
        const json reply = xfer(http + path + post + sign, apikey, crud);
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_object()
          or reply.at(0).find("symbol") == reply.at(0).end()
          or !reply.at(0).at("symbol").is_string()
          or reply.at(0).value("symbol", "") != base + quote
        ) {
          print("Error while reading fees: " + reply.dump());
          return json::object();
        }
        return reply.at(0);
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
    protected:
      string nonce() const override {
        return to_string(Tstamp);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/instrument/active");
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_object()
          or reply.at(0).find("symbol") == reply.at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply)
          report += it.value("symbol", "") + '\n';
      };
      json handshake() const override {
        json reply = {
          {"object", Curl::Web::xfer(http + "/instrument?symbol=" + base + quote)}
        };
        if (reply.at("object").is_array() and !reply.at("object").empty())
          reply = reply.at("object").at(0);
        return {
          {     "base", base                           },
          {    "quote", quote                          },
          {   "symbol", base + quote                   },
          {   "margin", reply.value("isInverse", false)
                          ? Future::Inverse
                          : Future::Linear             },
          {"webMarket", webMarket + base + quote       },
          {"webOrders", webOrders                      },
          {"tickPrice", reply.value("tickSize", 0.0)   },
          { "tickSize", reply.value("lotSize", 0.0)    },
          {  "minSize", reply.value("lotSize", 0.0)    },
          {  "makeFee", reply.value("makerFee", 0.0)   },
          {  "takeFee", reply.value("takerFee", 0.0)   },
          {    "reply", reply                          }
        };
      };
      json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &post, const string &crud) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("api-expires: "   + h1).data());
          h_ = curl_slist_append(h_, ("api-key: "       + h2).data());
          h_ = curl_slist_append(h_, ("api-signature: " + h3).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
        });
      };
  };
  class GwGateio: public GwApiWs {
    public:
      GwGateio()
      {
        http   = "https://api.gateio.ws/api/v4";
        ws     = "wss://api.gateio.ws/ws/v4/";
        randId = Random::int45Id;
        webMarket = "https://www.gate.io/trade/";
        webOrders = "https://www.gate.io/myaccount/myorders";
      };
    protected:
      string nonce() const override {
        return to_string((Clock)(Tstamp / 1e+3));
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/spot/currency_pairs");
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_object()
          or reply.at(0).find("trade_status") == reply.at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply)
          if (it.value("trade_status", "") == "tradable")
            report += it.value("base", "") + "/" + it.value("quote", "") + '\n';
      };
      json handshake() const override {
        json reply = {
          {"object", Curl::Web::xfer(http + "/spot/currency_pairs")}
        };
        if (reply.at("object").is_array() and !reply.at("object").empty())
          for (const json &it : reply.at("object"))
            if (it.value("id", "") == base + "_" + quote) {
              reply = it;
              break;
            }
        return {
          {     "base", base                                            },
          {    "quote", quote                                           },
          {   "symbol", base + "_" + quote                              },
          {"webMarket", webMarket + base + "_" + quote                  },
          {"webOrders", webOrders                                       },
          {"tickPrice", pow(10, -reply.value("precision", 0))           },
          { "tickSize", pow(10, -reply.value("amount_precision", 0))    },
          {  "minSize", stod(reply.value("min_base_amount", "0"))
                         ?: pow(10, -reply.value("amount_precision", 0))},
          { "minValue", stod(reply.value("min_quote_amount", "0"))
                         ?: pow(10, -reply.value("precision", 0))       },
          {  "makeFee", stod(reply.value("fee", "0")) / 1e+2            },
          {  "takeFee", stod(reply.value("fee", "0")) / 1e+2            },
          {    "reply", reply                                           }
        };
      };
      json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &post, const string &crud) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/json");
          h_ = curl_slist_append(h_, ("KEY: "       + h1).data());
          h_ = curl_slist_append(h_, ("Timestamp: " + h2).data());
          h_ = curl_slist_append(h_, ("SIGN: "      + h3).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
        });
      };
  };
  class GwHitBtc: public GwApiWsWs {
    public:
      GwHitBtc()
      {
        http   = "https://api.hitbtc.com/api/2";
        ws     = "wss://api.hitbtc.com/api/2/ws/public";
        randId = Random::uuid32Id;
        webMarket = "https://hitbtc.com/exchange/";
        webOrders = "https://hitbtc.com/reports/orders";
      };
    protected:
      string nonce() const override {
        return randId() + randId();
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/public/symbol");
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_object()
          or reply.at(0).find("baseCurrency")  == reply.at(0).end()
          or reply.at(0).find("quoteCurrency") == reply.at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply)
          report += it.value("baseCurrency", "") + "/" + it.value("quoteCurrency", "") + '\n';
      };
      json handshake() const override {
        const json reply = Curl::Web::xfer(http + "/public/symbol/" + base + quote);
        return {
          {     "base", base == "USDT" ? "USD" : base                 },
          {    "quote", quote == "USDT" ? "USD" : quote               },
          {   "symbol", base + quote                                  },
          {"webMarket", webMarket + base + "-to-" + quote             },
          {"webOrders", webOrders                                     },
          {"tickPrice", stod(reply.value("tickSize", "0"))            },
          { "tickSize", stod(reply.value("quantityIncrement", "0"))   },
          {  "minSize", stod(reply.value("quantityIncrement", "0"))   },
          {  "makeFee", stod(reply.value("provideLiquidityRate", "0"))},
          {  "takeFee", stod(reply.value("takeLiquidityRate", "0"))   },
          {    "reply", reply                                         }
        };
      };
      string twin(const string &ws) const override {
        return ws.substr(0, ws.length() - 6) + "trading";
      };
      json xfer(const string &url, const string &auth, const string &post) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          curl_easy_setopt(curl, CURLOPT_USERPWD, auth.data());
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
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
  class GwCoinbase: public GwApiWsFix {
    public:
      GwCoinbase()
        : GwApiWsFix("Coinbase")
      {
        http   = "https://api.pro.coinbase.com";
        ws     = "wss://ws-feed.pro.coinbase.com";
        fix    = "fix.pro.coinbase.com:4198";
        randId = Random::uuid36Id;
        webMarket = "https://pro.coinbase.com/trade/";
        webOrders = "https://pro.coinbase.com/orders/";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp / 1e+3);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/products");
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_object()
          or reply.at(0).find("base_currency")  == reply.at(0).end()
          or reply.at(0).find("quote_currency") == reply.at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply)
          if (!it.value("trading_disabled", true) and it.value("status", "") == "online")
          report += it.value("base_currency", "") + "/" + it.value("quote_currency", "") + '\n';
      };
      json handshake() const override {
        const json reply = Curl::Web::xfer(http + "/products/" + base + "-" + quote);
        return {
          {     "base", base                                     },
          {    "quote", quote                                    },
          {   "symbol", base + "-" + quote                       },
          {"webMarket", webMarket + base + quote                 },
          {"webOrders", webOrders + base + quote                 },
          {"tickPrice", stod(reply.value("quote_increment", "0"))},
          { "tickSize", stod(reply.value("base_increment", "0")) },
          {  "minSize", stod(reply.value("base_min_size", "0"))  },
          {    "reply", reply                                    }
        };
      };
      json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &h4, const string &crud) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("CB-ACCESS-KEY: "        + h1).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-SIGN: "       + h2).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-TIMESTAMP: "  + h3).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-PASSPHRASE: " + h4).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
        });
      };
  };
  class GwBitfinex: public GwApiWs {
    protected:
      string trading = "exchange";
    public:
      GwBitfinex()
      {
        http   = "https://api.bitfinex.com/v2";
        ws     = "wss://api.bitfinex.com/ws/2";
        randId = Random::int45Id;
        askForReplace = true;
        webMarket = "https://www.bitfinex.com/trading/";
        webOrders = "https://www.bitfinex.com/reports/orders";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp * 1e+3);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/conf/pub:list:pair:" + trading);
        if (!reply.is_array()
          or reply.empty()
          or !reply.at(0).is_array()
          or reply.at(0).empty()
          or !reply.at(0).at(0).is_string()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply.at(0))
          if (it.get<string>().find(":") != string::npos)
            report += it.get<string>().substr(0, it.get<string>().find(":"))  + "/"
                    + it.get<string>().substr(it.get<string>().find(":") + 1) + '\n';
          else
            report += it.get<string>().substr(0, 3) + "/"
                    + it.get<string>().substr(3)    + '\n';
      };
      json handshake() const override {
        json reply1 = {
          {"object", Curl::Web::xfer(http + "/ticker/t" + base + quote)}
        };
        if (reply1.at("object").is_array()
          and reply1.at("object").size() > 6
          and reply1.at("object").at(6).is_number()
        ) reply1["tickPrice"] = pow(10, fmax((int)log10(
            reply1.at("object").at(6).get<double>()
          ), -4) -4);
        json reply2 = {
          {"object", Curl::Web::xfer(http + "/conf/pub:info:pair")}
        };
        if (reply2.at("object").is_array() and !reply2.at("object").empty())
          for (const json &it : reply2.at("object").at(0)) {
            if (it.at(0).is_string()
              and it.at(0).get<string>() == base + quote
              and it.at(1).is_array()
              and it.at(1).size() > 3
              and it.at(1).at(3).is_string()
            ) {
              reply2 = {
                {"object", it}
              };
              reply2["minSize"] = stod(reply2.at("object").at(1).at(3).get<string>());
              break;
            }
          }
        return {
          {     "base", base                          },
          {    "quote", quote                         },
          {   "symbol", base + quote                  },
          {"webMarket", webMarket
                          + base + quote              },
          {"webOrders", webOrders                     },
          {"tickPrice", reply1.value("tickPrice", 0.0)},
          { "tickSize", 1e-8                          },
          {  "minSize", reply2.value("minSize", 0.0)  },
          {    "reply", {reply1, reply2}              }
        };
      };
      json xfer(const string &url, const string &post, const string &h1, const string &h2, const string &h3) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/json");
          h_ = curl_slist_append(h_, ("bfx-apikey: "    + h1).data());
          h_ = curl_slist_append(h_, ("bfx-nonce: "     + h2).data());
          h_ = curl_slist_append(h_, ("bfx-signature: " + h3).data());
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
  class GwKuCoin: public GwApiWs {
    public:
      GwKuCoin()
      {
        http   = "https://api.kucoin.com";
        ws     = "wss://push-private.kucoin.com/endpoint";
        randId = Random::uuid36Id;
        webMarket = "https://trade.kucoin.com/";
        webOrders = "https://www.kucoin.com/order/trade";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/api/v1/symbols");
        if (!reply.is_object()
          or reply.find("data") == reply.end()
          or !reply.at("data").is_array()
          or reply.at("data").empty()
          or !reply.at("data").at(0).is_object()
          or reply.at("data").at(0).find("enableTrading") == reply.at("data").at(0).end()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply.at("data"))
          if (it.value("enableTrading", false))
            report += it.value("baseCurrency", "") + "/" + it.value("quoteCurrency", "") + '\n';
      };
      json handshake() const override {
        json reply1 = Curl::Web::xfer(http + "/api/v1/symbols");
        if (reply1.find("data") != reply1.end() and reply1.at("data").is_array())
          for (const json &it : reply1.at("data"))
            if (it.value("symbol", "") == base + "-" + quote) {
              reply1 = it;
              break;
            }
        const json reply2 = fees();
        return {
          {     "base", base                                     },
          {    "quote", quote                                    },
          {   "symbol", base + "-" + quote                       },
          {"webMarket", webMarket + base + "-" + quote           },
          {"webOrders", webOrders                                },
          {"tickPrice", stod(reply1.value("priceIncrement", "0"))},
          { "tickSize", stod(reply1.value("baseIncrement", "0")) },
          {  "minSize", stod(reply1.value("baseMinSize", "0"))   },
          {  "makeFee", stod(reply2.value("makerFeeRate", "0"))  },
          {  "takeFee", stod(reply2.value("takerFeeRate", "0"))  },
          {    "reply", {reply1, reply2}                         }
        };
      };
      json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &h4, const string &crud, const string &post = "") const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/json");
          h_ = curl_slist_append(h_, ("KC-API-KEY: "        + h1).data());
          h_ = curl_slist_append(h_, ("KC-API-SIGN: "       + h2).data());
          h_ = curl_slist_append(h_, ("KC-API-PASSPHRASE: " + h3).data());
          h_ = curl_slist_append(h_, ("KC-API-TIMESTAMP: "  + h4).data());
          h_ = curl_slist_append(h_,  "KC-API-KEY-VERSION: 2");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, crud.data());
        });
      };
    private:
      json fees() const {
        const string crud = "GET",
                     path = "/api/v1/base-fee",
                     time = nonce(),
                     hash = time + crud + path,
                     sign = Text::B64(Text::HMAC256(hash, secret, true)),
                     code = Text::B64(Text::HMAC256(pass, secret, true));
        const json reply = xfer(http + path, apikey, sign, code, time, crud);
        if (reply.find("code") == reply.end()
          or !reply.at("code").is_string()
          or reply.value("code", "") != "200000"
          or reply.find("data") == reply.end()
          or !reply.at("data").is_object()
        ) {
          print("Error while reading fees: " + reply.dump());
          return json::object();
        }
        return reply.at("data");
      };
  };
  class GwKraken: public GwApiWsWs {
    public:
      GwKraken()
      {
        http   = "https://api.kraken.com";
        ws     = "wss://ws.kraken.com";
        randId = Random::int32Id;
        webMarket = "https://www.kraken.com/charts";
        webOrders = "https://www.kraken.com/u/trade";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/0/public/AssetPairs");
        if (!reply.is_object()
          or reply.find("result") == reply.end()
          or !reply.at("result").is_object()
        ) print("Error while reading pairs: " + reply.dump());
        else for (const json &it : reply.at("result"))
          if (it.find("wsname") != it.end())
            report += it.value("wsname", "") + '\n';
      };
      json handshake() const override {
        json reply = Curl::Web::xfer(http + "/0/public/AssetPairs?pair=" + base + quote);
        if (reply.find("result") != reply.end())
          for (const json &it : reply.at("result"))
            if (it.find("pair_decimals") != it.end()) {
              reply = it;
              break;
            }
        return {
          {     "base", base                                     },
          {    "quote", quote                                    },
          {   "symbol", reply.value("wsname", "")                },
          {"webMarket", webMarket                                },
          {"webOrders", webOrders                                },
          {"tickPrice", pow(10, -reply.value("pair_decimals", 0))},
          { "tickSize", pow(10, -reply.value("lot_decimals", 0)) },
          {  "minSize", pow(10, -reply.value("lot_decimals", 0)) },
          {    "reply", reply                                    }
        };
      };
      string twin(const string &ws) const override {
        return string(ws).insert(ws.find("ws.") + 2, "-auth");
      };
      json xfer(const string &url, const string &h1, const string &h2, const string &post) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("API-Key: "  + h1).data());
          h_ = curl_slist_append(h_, ("API-Sign: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwPoloniex: public GwApiWs {
    public:
      GwPoloniex()
      {
        http   = "https://poloniex.com";
        ws     = "wss://api2.poloniex.com";
        randId = Random::int45Id;
        webMarket = "https://poloniex.com/exchange";
        webOrders = "https://poloniex.com/tradeHistory";
      };
    protected:
      string nonce() const override {
        return to_string(Tstamp);
      };
      void pairs(string &report) const override {
        const json reply = Curl::Web::xfer(http + "/public?command=returnTicker");
        if (!reply.is_object())
          print("Error while reading pairs: " + reply.dump());
        else for (auto it = reply.begin(); it != reply.end(); ++it)
          report += it.key() + '\n';
      };
      json handshake() const override {
        const json reply = Curl::Web::xfer(http + "/public?command=returnTicker")
                             .value(quote + "_" + base, json::object());
        return {
          {     "base", base              },
          {    "quote", quote             },
          {   "symbol", quote + "_" + base},
          {"webMarket", webMarket         },
          {"webOrders", webOrders         },
          {"tickPrice", reply.empty()
                          ? 0 : 1e-8      },
          { "tickSize", 1e-8              },
          {  "minSize", 1e-4              },
          {    "reply", reply             }
        };
      };
      json xfer(const string &url, const string &post, const string &h1, const string &h2) const {
        return Curl::Web::xfer(url, [&](CURL *curl) {
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

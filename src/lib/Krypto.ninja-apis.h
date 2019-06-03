#ifndef K_APIS_H_
#define K_APIS_H_
//! \file
//! \brief Exchange API integrations.

namespace ₿ {
  enum class Connectivity: unsigned int { Disconnected, Connected };
  enum class       Status: unsigned int { Waiting, Working, Terminated };
  enum class         Side: unsigned int { Bid, Ask };
  enum class  TimeInForce: unsigned int { GTC, IOC, FOK };
  enum class    OrderType: unsigned int { Limit, Market };

  struct mLevel {
     Price price;
    Amount size;
  };
  static void to_json(json &j, const mLevel &k) {
    j = {
      {"price", k.price}
    };
    if (k.size) j["size"] = k.size;
  };
  struct mLevels {
    vector<mLevel> bids,
                   asks;
  };
  static void to_json(json &j, const mLevels &k) {
    j = {
      {"bids", k.bids},
      {"asks", k.asks}
    };
  };

  struct mWallet {
    CoinId currency;
    Amount amount,
           held,
           total,
           value;
    double profit;
    static void reset(const Amount &a, const Amount &h, mWallet *const wallet) {
      wallet->total = (wallet->amount = a)
                    + (wallet->held   = h);
    };
  };
  static void to_json(json &j, const mWallet &k) {
    j = {
      {"amount", k.amount},
      {  "held", k.held  },
      { "value", k.value },
      {"profit", k.profit}
    };
  };
  struct mWallets {
    mWallet base,
            quote;
  };
  static void to_json(json &j, const mWallets &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };

  struct mTrade {
      Side side;
     Price price;
    Amount quantity;
     Clock time;
  };
  static void to_json(json &j, const mTrade &k) {
    j = {
      {    "side", k.side    },
      {   "price", k.price   },
      {"quantity", k.quantity},
      {    "time", k.time    }
    };
  };

  struct mOrder: public mTrade {
           bool isPong;
         RandId orderId,
                exchangeId;
         Status status;
         Amount tradeQuantity;
      OrderType type;
    TimeInForce timeInForce;
           bool disablePostOnly;
          Clock latency;
    static void update(const mOrder &raw, mOrder *const order) {
      if (!order) return;
      if (Status::Working == (     order->status     = raw.status
      ) and !order->latency)       order->latency    = raw.time - order->time;
                                   order->time       = raw.time;
      if (!raw.exchangeId.empty()) order->exchangeId = raw.exchangeId;
      if (raw.price)               order->price      = raw.price;
      if (raw.quantity)            order->quantity   = raw.quantity;
    };
    static const bool replace(const Price &price, const bool &isPong, mOrder *const order) {
      if (!order
        or order->exchangeId.empty()
      ) return false;
      order->price  = price;
      order->isPong = isPong;
      order->time   = Tstamp;
      return true;
    };
    static const bool cancel(mOrder *const order) {
      if (!order
        or order->exchangeId.empty()
        or order->status == Status::Waiting
      ) return false;
      order->status = Status::Waiting;
      order->time   = Tstamp;
      return true;
    };
  };
  static void to_json(json &j, const mOrder &k) {
    j = {
      {        "orderId", k.orderId        },
      {     "exchangeId", k.exchangeId     },
      {           "side", k.side           },
      {       "quantity", k.quantity       },
      {           "type", k.type           },
      {         "isPong", k.isPong         },
      {          "price", k.price          },
      {    "timeInForce", k.timeInForce    },
      {         "status", k.status         },
      {"disablePostOnly", k.disablePostOnly},
      {           "time", k.time           },
      {        "latency", k.latency        }
    };
  };
  static void from_json(const json &j, mOrder &k) {
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
    k.disablePostOnly = true;
  };

  class GwExchangeData {
    public_friend:
      class Decimal {
        public:
          stringstream stream;
        public:
          Decimal()
          {
            stream << fixed;
          };
          const double round(const double &input) const {
            const double points = pow(10, -1 * stream.precision());
            return ::round(input / points) * points;
          };
          const double floor(const double &input) const {
            const double points = pow(10, -1 * stream.precision());
            return ::floor(input / points) * points;
          };
          const string str(const double &input) {
            stream.str("");
            stream << round(input);
            return stream.str();
          };
      };
    public:
      struct {
        Decimal price,
                amount,
                percent;
      } decimal;
      function<void(const mOrder&)>       write_mOrder;
      function<void(const mTrade&)>       write_mTrade;
      function<void(const mLevels&)>      write_mLevels;
      function<void(const mWallets&)>     write_mWallets;
      function<void(const Connectivity&)> write_Connectivity;
      bool askForFees      = false,
           askForReplace   = false,
           askForCancelAll = false;
      const RandId (*randId)() = nullptr;
      virtual const bool askForData(const unsigned int &tick) = 0;
      virtual const bool waitForData() = 0;
      void online(const Connectivity &connectivity = Connectivity::Connected) {
        if (write_Connectivity)
          write_Connectivity(connectivity);
      };
      void place(const mOrder *const order) {
        place(
          order->orderId,
          order->side,
          decimal.price.str(order->price),
          decimal.amount.str(order->quantity),
          order->type,
          order->timeInForce,
          order->disablePostOnly
        );
      };
      void replace(const mOrder *const order) {
        replace(
          order->exchangeId,
          decimal.price.str(order->price)
        );
      };
      void cancel(const mOrder *const order) {
        cancel(
          order->orderId,
          order->exchangeId
        );
      };
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual bool ready(uS::Loop *const) = 0;                               // wait for exchange and register data handlers
/**/  virtual void replace(RandId, string) {};                               // call         async orders data from exchange
/**/  virtual void place(RandId, Side, string, string, OrderType, TimeInForce, bool) = 0; // async orders like above / below
/**/  virtual void cancel(RandId, RandId) = 0;                               // call         async orders data from exchange
/**/protected:
/**/  virtual bool            async_wallet() { return false; };              // call         async wallet data from exchange
/**/  virtual vector<mWallets> sync_wallet() { return {}; };                 // call and read sync wallet data from exchange
/**/  virtual vector<mLevels>  sync_levels() { return {}; };                 // call and read sync levels data from exchange
/**/  virtual vector<mTrade>   sync_trades() { return {}; };                 // call and read sync trades data from exchange
/**/  virtual vector<mOrder>   sync_orders() { return {}; };                 // call and read sync orders data from exchange
/**/  virtual vector<mOrder>   sync_cancelAll() = 0;                         // call and read sync orders data from exchange
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      future<vector<mWallets>> replyWallets;
      future<vector<mLevels>> replyLevels;
      future<vector<mTrade>> replyTrades;
      future<vector<mOrder>> replyOrders;
      future<vector<mOrder>> replyCancelAll;
      const bool askForNeverAsyncData(const unsigned int &tick) {
        bool waiting = false;
        if ((askForFees
          and !(askForFees = false)
          ) or !(tick % 15)) waiting |= !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); }));
        if (askForCancelAll
          and !(tick % 300)) waiting |= askFor(replyCancelAll, [&]() { return sync_cancelAll(); });
        return waiting;
      };
      const bool askForSyncData(const unsigned int &tick) {
        bool waiting = false;
        if (!(tick % 2))     waiting |= askFor(replyOrders, [&]() { return sync_orders(); });
                             waiting |= askForNeverAsyncData(tick);
        if (!(tick % 3))     waiting |= askFor(replyLevels, [&]() { return sync_levels(); });
        if (!(tick % 60))    waiting |= askFor(replyTrades, [&]() { return sync_trades(); });
        return waiting;
      };
      const bool waitForNeverAsyncData() {
        return waitFor(replyWallets,   write_mWallets)
             | waitFor(replyCancelAll, write_mOrder);
      };
      const bool waitForSyncData() {
        return waitFor(replyOrders,    write_mOrder)
             | waitForNeverAsyncData()
             | waitFor(replyLevels,    write_mLevels)
             | waitFor(replyTrades,    write_mTrade);
      };
      template<typename T1, typename T2> const bool askFor(
              future<vector<T1>> &reply,
        const T2                 &read
      ) {
        bool waiting = reply.valid();
        if (!waiting) {
          reply = ::async(launch::async, read);
          waiting = true;
        }
        return waiting;
      };
      template<typename T> const unsigned int waitFor(
              future<vector<T>>        &reply,
        const function<void(const T&)> &write
      ) {
        bool waiting = reply.valid();
        if (waiting and reply.wait_for(chrono::nanoseconds(0)) == future_status::ready) {
          for (T &it : reply.get()) write(it);
          waiting = false;
        }
        return waiting;
      };
  };

  class GwExchange: public GwExchangeData {
    public:
      using Report = vector<pair<string, string>>;
        string exchange, apikey,
               secret,   pass,
               http,     ws,
               fix,      unlock;
        CoinId base,     quote;
           int version  = 0,
               maxLevel = 0,
               debug    = 0;
         Price minTick  = 0;
        Amount minSize  = 0,
               makeFee  = 0,
               takeFee  = 0;
      virtual const bool network()   = 0;
      virtual const json handshake() = 0;
      const json handshake(const bool &nocache) {
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
        minTick = reply.value("minTick", 0.0);
        if (!minSize) minSize = reply.value("minSize", 0.0);
        if (!makeFee) makeFee = reply.value("makeFee", 0.0);
        if (!takeFee) takeFee = reply.value("takeFee", 0.0);
        if (!file.is_open() and minTick and minSize) {
          file.open(cache, fstream::out | fstream::trunc);
          file << reply.dump();
        }
        if (file.is_open()) file.close();
        return reply.value("reply", json::object());
      };
      virtual void close() {
        online(Connectivity::Disconnected);
      };
      void end(const bool &dustybot = false) {
        if (dustybot)
          log("--dustybot is enabled, remember to cancel manually any open order.");
        else if (write_mOrder) {
          log("Attempting to cancel all open orders, please wait.");
          for (mOrder &it : sync_cancelAll()) write_mOrder(it);
          log("cancel all open orders OK");
        }
        close();
      };
      void report(Report notes, const bool &nocache) {
        decimal.price.stream.precision(abs(log10(minTick)));
        decimal.amount.stream.precision(minTick < 1e-8 ? 10 : 8);
        decimal.percent.stream.precision(2);
        for (auto it : (Report){
          {"symbols", base + "/" + quote},
          {"minTick", decimal.amount.str(minTick)              },
          {"minSize", decimal.amount.str(minSize)              },
          {"makeFee", decimal.percent.str(makeFee * 1e+2) + "%"},
          {"takeFee", decimal.percent.str(takeFee * 1e+2) + "%"}
        }) notes.push_back(it);
        string note = "handshake:";
        for (auto &it : notes)
          if (!it.second.empty())
            note += "\n- " + it.first + ": " + it.second;
        log((nocache ? "" : "cached ") + note);
      };
      void latency(const string &reason, const function<void()> &fn) {
        log("latency check", "start");
        const Clock Tstart = Tstamp;
        fn();
        const Clock Tstop  = Tstamp;
        log("latency check", "stop");
        const unsigned int Tdiff = Tstop - Tstart;
        log(reason + " took", to_string(Tdiff) + "ms of your time");
        string result = "This result is ";
        if      (Tdiff < 2e+2) result += "very good; most traders don't enjoy such speed!";
        else if (Tdiff < 5e+2) result += "good; most traders get the same result";
        else if (Tdiff < 7e+2) result += "a bit bad; most traders get better results";
        else if (Tdiff < 1e+3) result += "bad; consider moving to another server/network";
        else                   result += "very bad; move to another server/network";
        log(result);
      };
      function<void(const string&, const string&, const string&)> printer;
    protected:
      void log(const string &reason, const string &highlight = "") {
        if (printer) printer(
          string(reason.find(">>>") != reason.find("<<<")
            ? "DEBUG "
            : "GW "
          ) + exchange,
          reason,
          highlight
        );
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
      const bool network() override {
        return true;
      };
      const bool askForData(const unsigned int &tick) override {
        return askForSyncData(tick);
      };
      const bool waitForData() override {
        return waitForSyncData();
      };
  };
  class GwApiWS: public Gw {
    private:
               CURL *curl   = nullptr;
      curl_socket_t  sockfd = 0;
             string  buffer;
       unsigned int  countdown = 1;
               bool  subscription = false;
    public:
      const bool network() override {
        return sockfd
           and !countdown;
      };
      const bool askForData(const unsigned int &tick) override {
        if (connected() and subscribed())
          askForNeverAsyncData(tick);
        return true;
      };
      const bool waitForData() override {
        if (subscribed()) {
          waitForAsyncData();
          waitForNeverAsyncData();
        }
        return true;
      };
    protected:
//BO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual void consume(json) = 0;                                         // read message one by one from remote server.
/**/  virtual void subscribe()   = 0;                                         // send subcription messages to remote server.
//EO non-free Gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      void unsubscribe() {
        GwExchange::close();
        reconnect("Disconnected");
      };
      void send(const string &msg) {
        CURLcode rc;
        if (CURLE_OK != (rc = Curl::Ws::emit(curl, sockfd, msg, 0x01)))
          GwExchange::log(string("CURL send Error: ") + curl_easy_strerror(rc));
      };
      void disconnect() {
        Curl::Ws::emit(curl, sockfd, "", 0x08);
        Curl::Ws::cleanup(curl, sockfd);
      };
      void close() override {
        GwExchange::close();
        countdown = ANY_NUM;
        disconnect();
      };
      void reconnect(const string &reason) {
        if (countdown == ANY_NUM) return;
        countdown = 7;
        GwExchange::log("WS " + reason + ", reconnecting in " + to_string(countdown) + "s.");
      };
    private:
      void waitForAsyncData() {
        if (received())
          for (;;) {
            string msg;
            Curl::Ws::unframe(curl, sockfd, buffer, msg);
            if (msg.empty()) break;
            consume(
              json::accept(msg)
                ? json::parse(msg)
                : (json){ {"error", "CURL Error: Unsupported frame data format"} }
            );
          }
      };
      const bool connected() {
        if (countdown and !--countdown) {
          CURLcode rc;
          if (CURLE_OK != (rc = Curl::Ws::connect(curl, sockfd, buffer, ws)))
            reconnect(string("CURL connect Error: ") + curl_easy_strerror(rc));
        }
        return network();
      };
      const bool received() {
        CURLcode rc;
        if (CURLE_OK != (rc = Curl::Ws::receive(curl, sockfd, buffer)))
          GwExchange::log(string("CURL recv Error: ") + curl_easy_strerror(rc));
        return !buffer.empty();
      };
      const bool subscribed() {
        if (subscription != network()) {
          subscription = !subscription;
          if (subscription) subscribe();
          else unsubscribe();
        }
        return subscription;
      };
  };

  class GwNull: public GwApiREST {
    public:
      GwNull()
      {
        randId = Random::uuid36Id;
      };
    public:
      const json handshake() override {
        return {
          {"minTick", 1e-2   },
          {"minSize", 1e-2   },
          {  "reply", nullptr}
        };
      };
  };
  class GwHitBtc: public GwApiWS {
    public:
      GwHitBtc()
      {
        http   = "https://api.hitbtc.com/api/2";
        ws     = "wss://api.hitbtc.com/api/2/ws";
        randId = Random::uuid32Id;
      };
      const json handshake() override {
        const json reply = Curl::Http::xfer(http + "/public/symbol/" + base + quote);
        return {
          {"minTick", stod(reply.value("tickSize", "0"))            },
          {"minSize", stod(reply.value("quantityIncrement", "0"))   },
          {"makeFee", stod(reply.value("provideLiquidityRate", "0"))},
          {"takeFee", stod(reply.value("takeLiquidityRate", "0"))   },
          {  "reply", reply                                         }
        };
      };
    protected:
      static const json xfer(const string &url, const string &auth, const string &post) {
        return Curl::Http::request(url, [&](CURL *curl) {
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
      };
  };
  class GwCoinbase: public GwApiWS,
                    public FIX::NullApplication {
    public:
      GwCoinbase()
      {
        http   = "https://api.pro.coinbase.com";
        ws     = "wss://ws-feed.pro.coinbase.com";
        fix    = "fix.pro.coinbase.com:4198";
        randId = Random::uuid36Id;
      };
      const json handshake() override {
        const json reply = Curl::Http::xfer(http + "/products/" + base + "-" + quote);
        return {
          {"minTick", stod(reply.value("quote_increment", "0"))},
          {"minSize", stod(reply.value("base_min_size", "0"))  },
          {  "reply", reply                                    }
        };
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &h4, const bool &rm) {
        return Curl::Http::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("CB-ACCESS-KEY: " + h1).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-SIGN: " + h2).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-TIMESTAMP: " + h3).data());
          h_ = curl_slist_append(h_, ("CB-ACCESS-PASSPHRASE: " + h4).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          if (rm) curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        });
      };
  };
  class GwBitfinex: public GwApiWS {
    public:
      GwBitfinex()
      {
        http   = "https://api.bitfinex.com/v1";
        ws     = "wss://api.bitfinex.com/ws/2";
        randId = Random::int45Id;
        askForReplace = true;
      };
      const json handshake() override {
        const json reply1 = Curl::Http::xfer(http + "/pubticker/" + base + quote);
        Price minTick = 0,
              minSize = 0;
        if (reply1.find("last_price") != reply1.end()) {
          ostringstream price_;
          price_ << scientific << stod(reply1.value("last_price", "0"));
          string price = price_.str();
          for (string::iterator it=price.begin(); it!=price.end();)
            if (*it == '+' or *it == '-') break;
            else it = price.erase(it);
          istringstream iss("1e" + to_string(fmax(stod(price),-4)-4));
          iss >> minTick;
        }
        const json reply2 = Curl::Http::xfer(http + "/symbols_details");
        if (reply2.is_array())
          for (const json &it : reply2)
            if (it.find("pair") != it.end() and it.value("pair", "") == Text::strL(base + quote)) {
              minSize = stod(it.value("minimum_order_size", "0"));
              break;
            }
        return {
          {"minTick", minTick         },
          {"minSize", minSize         },
          {  "reply", {reply1, reply2}}
        };
      };
    protected:
      static const json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::Http::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("X-BFX-APIKEY: " + h1).data());
          h_ = curl_slist_append(h_, ("X-BFX-PAYLOAD: " + post).data());
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
      };
  };
  class GwFCoin: public GwApiWS {
    public:
      GwFCoin()
      {
        http   = "https://api.fcoin.com/v2/";
        ws     = "wss://api.fcoin.com/v2/ws";
        randId = Random::char16Id;
      };
      const json handshake() override {
        const json reply = Curl::Http::xfer(http + "public/symbols");
        Price minTick = 0,
              minSize = 0;
        if (reply.find("data") != reply.end() and reply.at("data").is_array())
          for (const json &it : reply.at("data"))
            if (it.find("name") != it.end() and it.value("name", "") == Text::strL(base + quote)) {
              istringstream iss(
                "1e-" + to_string(it.value("price_decimal", 0))
                + " 1e-" + to_string(it.value("amount_decimal", 0))
              );
              iss >> minTick >> minSize;
              break;
            }
        return {
          {"minTick", minTick},
          {"minSize", minSize},
          {  "reply", reply  }
        };
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &post = "") {
        return Curl::Http::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          if (!post.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
            h_ = curl_slist_append(h_, "Content-Type: application/json;charset=UTF-8");
          }
          h_ = curl_slist_append(h_, ("FC-ACCESS-KEY: " + h1).data());
          h_ = curl_slist_append(h_, ("FC-ACCESS-SIGNATURE: " + h2).data());
          h_ = curl_slist_append(h_, ("FC-ACCESS-TIMESTAMP: " + h3).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        });
      };
  };
  class GwKraken: public GwApiREST {
    public:
      GwKraken()
      {
        http   = "https://api.kraken.com";
        randId = Random::int32Id;
      };
      const json handshake() override {
        const json reply = Curl::Http::xfer(http + "/0/public/AssetPairs?pair=" + base + quote);
        Price minTick = 0,
              minSize = 0;
        if (reply.find("result") != reply.end())
          for (json::const_iterator it = reply.at("result").cbegin(); it != reply.at("result").cend(); ++it)
            if (it.value().find("pair_decimals") != it.value().end()) {
              istringstream iss(
                "1e-" + to_string(it.value().value("pair_decimals", 0))
                + " 1e-" + to_string(it.value().value("lot_decimals", 0))
              );
              iss >> minTick >> minSize;
              break;
            }
        return {
          {"minTick", minTick},
          {"minSize", minSize},
          {  "reply", reply  }
        };
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &post) {
        return Curl::Http::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("API-Key: " + h1).data());
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
      };
      const json handshake() override {
        const json reply = Curl::Http::xfer(http + "/public?command=returnTicker")
                             .value(quote + "_" + base, json::object());
        return {
          {"minTick", reply.empty()
                        ? 0
                        : 1e-8     },
          {"minSize", 1e-3         },
          {  "reply", reply        }
        };
      };
    protected:
      static const json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::Http::request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          h_ = curl_slist_append(h_, ("Key: " + h1).data());
          h_ = curl_slist_append(h_, ("Sign: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
}

#endif

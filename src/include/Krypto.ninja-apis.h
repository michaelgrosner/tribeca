#ifndef K_APIS_H_
#define K_APIS_H_
//! \file
//! \brief External exchange API integrations.

namespace ₿ {
  enum class Connectivity: unsigned int {
    Disconnected, Connected
  };
  enum class Status: unsigned int {
    Waiting, Working, Terminated
  };
  enum class Side: unsigned int {
    Bid, Ask
  };
  enum class TimeInForce: unsigned int {
    IOC, FOK, GTC
  };
  enum class OrderType: unsigned int {
    Limit, Market
  };

  struct mOrder {
         RandId orderId,
                exchangeId;
         Status status         = Status::Waiting;
           Side side           = (Side)0;
          Price price          = 0;
         Amount quantity       = 0,
                tradeQuantity  = 0;
      OrderType type           = OrderType::Limit;
    TimeInForce timeInForce    = TimeInForce::GTC;
           bool isPong         = false,
                preferPostOnly = true;
          Clock time           = 0,
                latency        = 0;
    mOrder() = default;
    mOrder(const RandId &o, const Side &s, const Price &p, const Amount &q, const bool &i)
      : orderId(o)
      , side(s)
      , price(p)
      , quantity(q)
      , isPong(i)
      , time(Tstamp)
    {};
    mOrder(const RandId &o, const RandId &e, const Status &s, const Price &p, const Amount &q, const Amount &Q)
      : orderId(o)
      , exchangeId(e)
      , status(s)
      , price(p)
      , quantity(q)
      , tradeQuantity(Q)
      , time(Tstamp)
    {};
    static void update(const mOrder &raw, mOrder *const order) {
      if (!order) return;
      if (Status::Working == (     order->status     = raw.status
      ) and !order->latency)       order->latency    = Tstamp - order->time;
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
      {       "orderId", k.orderId       },
      {    "exchangeId", k.exchangeId    },
      {          "side", k.side          },
      {      "quantity", k.quantity      },
      {          "type", k.type          },
      {        "isPong", k.isPong        },
      {         "price", k.price         },
      {   "timeInForce", k.timeInForce   },
      {        "status", k.status        },
      {"preferPostOnly", k.preferPostOnly},
      {          "time", k.time          },
      {       "latency", k.latency       }
    };
  };
  static void from_json(const json &j, mOrder &k) {
    k.price          = j.value("price", 0.0);
    k.quantity       = j.value("quantity", 0.0);
    k.side           = j.value("side", "") == "Bid"
                         ? Side::Bid
                         : Side::Ask;
    k.type           = j.value("type", "") == "Limit"
                         ? OrderType::Limit
                         : OrderType::Market;
    k.timeInForce    = j.value("timeInForce", "") == "GTC"
                         ? TimeInForce::GTC
                         : (j.value("timeInForce", "") == "FOK"
                           ? TimeInForce::FOK
                           : TimeInForce::IOC);
    k.isPong         = false;
    k.preferPostOnly = false;
  };

  struct mTrade {
     string tradeId;
       Side side         = (Side)0;
      Price price        = 0,
            Kprice       = 0;
     Amount quantity     = 0,
            value        = 0,
            Kqty         = 0,
            Kvalue       = 0,
            Kdiff        = 0,
            feeCharged   = 0;
      Clock time         = 0,
            Ktime        = 0;
       bool isPong       = false,
            loadedFromDB = false;
    mTrade() = default;
    mTrade(const Price p, const Amount q, const Side s, const Clock t)
      : side(s)
      , price(p)
      , quantity(q)
      , time(t)
    {};
    mTrade(const Price &p, const Amount &q, const Side &S, const bool &P, const Clock &t, const Amount &v, const Clock &Kt, const Amount &Kq, const Price &Kp, const Amount &Kv, const Amount &Kd, const Amount &f, const bool &l)
      : tradeId(to_string(t))
      , side(S)
      , price(p)
      , Kprice(Kp)
      , quantity(q)
      , value(v)
      , Kqty(Kq)
      , Kvalue(Kv)
      , Kdiff(Kd)
      , feeCharged(f)
      , time(t)
      , Ktime(Kt)
      , isPong(P)
      , loadedFromDB(l)
    {};
  };
  static void to_json(json &j, const mTrade &k) {
    if (k.tradeId.empty()) j = {
      {    "time", k.time    },
      {   "price", k.price   },
      {"quantity", k.quantity},
      {    "side", k.side    }
    };
    else j = {
      {     "tradeId", k.tradeId     },
      {        "time", k.time        },
      {       "price", k.price       },
      {    "quantity", k.quantity    },
      {        "side", k.side        },
      {       "value", k.value       },
      {       "Ktime", k.Ktime       },
      {        "Kqty", k.Kqty        },
      {      "Kprice", k.Kprice      },
      {      "Kvalue", k.Kvalue      },
      {       "Kdiff", k.Kdiff       },
      {  "feeCharged", k.feeCharged  },
      {      "isPong", k.isPong      },
      {"loadedFromDB", k.loadedFromDB}
    };
  };
  static void from_json(const json &j, mTrade &k) {
    k.tradeId      = j.value("tradeId",     "");
    k.price        = j.value("price",      0.0);
    k.quantity     = j.value("quantity",   0.0);
    k.side         = j.value("side",   (Side)0);
    k.time         = j.value("time",  (Clock)0);
    k.value        = j.value("value",      0.0);
    k.Ktime        = j.value("Ktime", (Clock)0);
    k.Kqty         = j.value("Kqty",       0.0);
    k.Kprice       = j.value("Kprice",     0.0);
    k.Kvalue       = j.value("Kvalue",     0.0);
    k.Kdiff        = j.value("Kdiff",      0.0);
    k.feeCharged   = j.value("feeCharged", 0.0);
    k.isPong       = j.value("isPong",   false);
    k.loadedFromDB = true;
  };

  struct mLevel {
     Price price = 0;
    Amount size  = 0;
    mLevel() = default;
    mLevel(const Price &p, const Amount &s)
      : price(p)
      , size(s)
    {};
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
    Amount amount = 0,
           held   = 0,
           total  = 0,
           value  = 0,
           profit = 0;
    CoinId currency;
    mWallet() = default;
    mWallet(const Amount &a, const Amount &h, const CoinId &c)
      : amount(a)
      , held(h)
      , currency(c)
    {};
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

  class Curl {
    private:
      static mutex waiting_reply;
    public:
      static const char *inet;
      static const json xfer(const string &url, const long &timeout = 13) {
        return perform(url, [&](CURL *curl) {
          curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        }, timeout == 13);
      };
      static const json xfer(const string &url, const string &post) {
        return perform(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
      static const json perform(const string &url, function<void(CURL *curl)> setopt, bool debug = true) {
        lock_guard<mutex> lock(waiting_reply);
        string reply;
        CURLcode res = CURLE_FAILED_INIT;
        CURL *curl = curl_easy_init();
        if (curl) {
          setopt(curl);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          curl_easy_setopt(curl, CURLOPT_INTERFACE, inet);
          curl_easy_setopt(curl, CURLOPT_URL, url.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
          res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
        }
        return (debug and res != CURLE_OK)
          ? (json){ {"error", string("CURL Error: ") + curl_easy_strerror(res)} }
          : (json::accept(reply)
              ? json::parse(reply)
              : json::object()
            );
      };
    private:
      static size_t write(void *buf, size_t size, size_t nmemb, void *reply) {
        ((string*)reply)->append((char*)buf, size *= nmemb);
        return size;
      };
  };

  class Text {
    public:
      static string strL(string input) {
        transform(input.begin(), input.end(), input.begin(), ::tolower);
        return input;
      };
      static string strU(string input) {
        transform(input.begin(), input.end(), input.begin(), ::toupper);
        return input;
      };
      static string B64(const string &input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.data(), input.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        BIO_set_close(bio, BIO_NOCLOSE);
        BIO_free_all(bio);
        return string(bufferPtr->data, bufferPtr->length);
      };
      static string B64_decode(const string &input) {
        BIO *bio, *b64;
        char buffer[input.length()];
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(input.data(), input.length());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_set_close(bio, BIO_NOCLOSE);
        int len = BIO_read(bio, buffer, input.length());
        BIO_free_all(bio);
        return string(buffer, len);
      };
      static string SHA256(const string &input, const bool &hex = false) {
        return SHA(input, hex, ::SHA256, SHA256_DIGEST_LENGTH);
      };
      static string SHA512(const string &input) {
        return SHA(input, false, ::SHA512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC1(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha1, SHA_DIGEST_LENGTH);
      };
      static string HMAC256(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha256, SHA256_DIGEST_LENGTH);
      };
      static string HMAC512(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC384(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha384, SHA384_DIGEST_LENGTH);
      };
    private:
      static string SHA(
        const string  &input,
        const bool    &hex,
        unsigned char *(*md)(const unsigned char*, size_t, unsigned char*),
        const int     &digest_len
      ) {
        unsigned char digest[digest_len];
        md((unsigned char*)input.data(), input.length(), (unsigned char*)&digest);
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HMAC(
        const string &key,
        const string &input,
        const bool   &hex,
        const EVP_MD *(evp_md)(),
        const int    &digest_len
      ) {
        unsigned char* digest;
        digest = ::HMAC(
          evp_md(),
          input.data(), input.length(),
          (unsigned char*)key.data(), key.length(),
          nullptr, nullptr
        );
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HEX(const string &input) {
        const unsigned int len = input.length();
        string output;
        for (unsigned int i = 0; i < len; i += 2)
          output.push_back(
            (char)(int)strtol(input.substr(i, 2).data(), nullptr, 16)
          );
        return output;
      };
  };

  class Random {
    public:
      static const unsigned long long int64() {
        static random_device rd;
        static mt19937_64 gen(rd());
        return uniform_int_distribution<unsigned long long>()(gen);
      };
      static const RandId int45Id() {
        return to_string(int64()).substr(0, 10);
      };
      static const RandId int32Id() {
        return to_string(int64()).substr(0,  8);
      };
      static const RandId char16Id() {
        string id = string(16, ' ');
        for (auto &it : id) {
         const int offset = int64() % (26 + 26 + 10);
         if (offset < 26)           it = 'a' + offset;
         else if (offset < 26 + 26) it = 'A' + offset - 26;
         else                       it = '0' + offset - 26 - 26;
        }
        return id;
      };
      static const RandId uuid36Id() {
        string uuid = string(36, ' ');
        uuid[8]  = '-';
        uuid[13] = '-';
        uuid[18] = '-';
        uuid[23] = '-';
        uuid[14] = '4';
        unsigned long long rnd = int64();
        for (auto &it : uuid)
          if (it == ' ') {
            if (rnd <= 0x02) rnd = 0x2000000 + (int64() * 0x1000000) | 0;
            rnd >>= 4;
            const int offset = (uuid[17] != ' ' and uuid[19] == ' ')
              ? ((rnd & 0xf) & 0x3) | 0x8
              : rnd & 0xf;
            if (offset < 10) it = '0' + offset;
            else             it = 'a' + offset - 10;
          }
        return uuid;
      };
      static const RandId uuid32Id() {
        RandId uuid = uuid36Id();
        uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
        return uuid;
      }
  };

  class GwExchangeData {
    public:
      function<void(const mOrder&)>        write_mOrder;
      function<void(const mTrade&)>        write_mTrade;
      function<void(const mLevels&)>       write_mLevels;
      function<void(const mWallets&)>      write_mWallets;
      function<void(const Connectivity&)>  write_Connectivity;
#define RAWDATA_ENTRY_POINT(mData, read) write_##mData = [&](const mData &rawdata) read
      bool askForFees    = false,
           askForReplace = false;
      const bool *askForCancelAll = nullptr;
      const RandId (*randId)() = nullptr;
      virtual const bool askForData(const unsigned int &tick) = 0;
      virtual const bool waitForData() = 0;
      const double dec(const double &input, const unsigned int &precision = 0) {
        const double points = pow(10, -1 * (precision ?: decimal.precision()));
        return round(input / points) * points;
      };
      const string str(const double &input) {
        decimal.str("");
        decimal << input;
        return decimal.str();
      };
      void place(const mOrder *const order) {
        place(
          order->orderId,
          order->side,
          str(order->price),
          str(order->quantity),
          order->type,
          order->timeInForce,
          order->preferPostOnly
        );
      };
      void replace(const mOrder *const order) {
        replace(
          order->exchangeId,
          str(order->price)
        );
      };
      void cancel(const mOrder *const order) {
        cancel(
          order->orderId,
          order->exchangeId
        );
      };
//BO non-free gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members below).
/**/  virtual bool ready() = 0;                                              // wait for exchange and register data handlers
/**/  virtual void replace(RandId, string) {};                               // call         async orders data from exchange
/**/  virtual void place(RandId, Side, string, string, OrderType, TimeInForce, bool) = 0,  // async orders, like above/below
/**/               cancel(RandId, RandId) = 0,                               // call         async orders data from exchange
/**/               close() = 0;                                              // disconnect but without waiting for reconnect
/**/protected:
/**/  virtual bool            async_wallet() { return false; };              // call         async wallet data from exchange
/**/  virtual vector<mWallets> sync_wallet() { return {}; };                 // call and read sync wallet data from exchange
/**/  virtual vector<mLevels>  sync_levels()  { return {}; };                // call and read sync levels data from exchange
/**/  virtual vector<mTrade>   sync_trades()  { return {}; };                // call and read sync trades data from exchange
/**/  virtual vector<mOrder>   sync_orders()  { return {}; };                // call and read sync orders data from exchange
/**/  virtual vector<mOrder>   sync_cancelAll() = 0;                         // call and read sync orders data from exchange
//EO non-free gw library functions from build-*/local/lib/K-*.a (it just redefines all virtual gateway class members above).
      future<vector<mWallets>> replyWallets;
      future<vector<mLevels>> replyLevels;
      future<vector<mTrade>> replyTrades;
      future<vector<mOrder>> replyOrders;
      future<vector<mOrder>> replyCancelAll;
      const bool askForNeverAsyncData(const unsigned int &tick) {
        bool waiting = false;
        if (TRUEONCE(askForFees)
          or !(tick % 15))       waiting |= !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); }));
        if (askForCancelAll
          and *askForCancelAll
          and !(tick % 300))     waiting |= askFor(replyCancelAll, [&]() { return sync_cancelAll(); });
        return waiting;
      };
      const bool askForSyncData(const unsigned int &tick) {
        bool waiting = false;
        if (!(tick % 2))         waiting |= askFor(replyOrders, [&]() { return sync_orders(); });
                                 waiting |= askForNeverAsyncData(tick);
        if (!(tick % 3))         waiting |= askFor(replyLevels, [&]() { return sync_levels(); });
        if (!(tick % 60))        waiting |= askFor(replyTrades, [&]() { return sync_trades(); });
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
      template<typename mData, typename syncFn> const bool askFor(
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
      template<typename mData> const unsigned int waitFor(
              future<vector<mData>>        &reply,
        const function<void(const mData&)> &write
      ) {
        bool waiting = reply.valid();
        if (waiting and reply.wait_for(chrono::nanoseconds(0)) == future_status::ready) {
          for (mData &it : reply.get()) write(it);
          waiting = false;
        }
        return waiting;
      };
      stringstream decimal;
  };

  class GwExchange: public GwExchangeData {
    public:
      uWS::Hub *socket = nullptr;
      unsigned int countdown = 0;
        string exchange, symbol,
               apikey,   secret,
               user,     pass,
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
      virtual const json handshake() = 0;
      void connect() {
        socket->connect(ws, nullptr, {}, 5e+3, &socket->getDefaultGroup<uWS::CLIENT>());
      };
      void run() {
        socket->run();
      };
      void end(const bool &dustybot = false) {
        if (dustybot)
          log("--dustybot is enabled, remember to cancel manually any open order.");
        else if (write_mOrder) {
          log("Attempting to cancel all open orders, please wait.");
          for (mOrder &it : sync_cancelAll()) write_mOrder(it);
          log("cancel all open orders OK");
        }
      };
      void info(vector<pair<string, string>> notes) {
        if (exchange != "NULL") log("allows client IP");
        decimal << fixed;
        decimal.precision(minTick < 1e-8 ? 10 : 8);
        for (pair<string, string> it : (vector<pair<string, string>>){
          {"symbols", symbol      },
          {"minTick", str(minTick)},
          {"minSize", str(minSize)},
          {"makeFee", str(makeFee)},
          {"takeFee", str(takeFee)}
        }) notes.push_back(it);
        string info = "handshake:";
        for (pair<string, string> &it : notes)
          if (it.first != "gateway" or !it.second.empty())
            info += "\n- " + it.first + ": " + it.second;
        log(info);
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
      void reconnect(const string &reason) {
        countdown = 7;
        log("WS " + reason + ", reconnecting in " + to_string(countdown) + "s.");
      };
  };

  static class Gw: public GwExchange {
    public:
//BO non-free gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on argument).
/**/  static Gw* new_Gw(const string&); // may return too a nullptr instead of a child gateway class, if string is unknown..
//EO non-free gw library functions from build-*/local/lib/K-*.a (it just returns a derived gateway class based on argument).
  } *gw = nullptr;

  class GwApiREST: public Gw {
    public:
      const bool askForData(const unsigned int &tick) override {
        return askForSyncData(tick);
      };
      const bool waitForData() override {
        return waitForSyncData();
      };
  };
  class GwApiWS: public Gw {
    public:
      GwApiWS()
      { countdown = 1; };
      const bool askForData(const unsigned int &tick) override {
        return askForNeverAsyncData(tick);
      };
      const bool waitForData() override {
        return waitForNeverAsyncData();
      };
  };

  class GwNull: public GwApiREST {
    public:
      const json handshake() override {
        randId  = Random::uuid36Id;
        symbol  = base + "_" + quote;
        minTick = 0.01;
        minSize = 0.01;
        return nullptr;
      };
  };
  class GwHitBtc: public GwApiWS {
    public:
      GwHitBtc()
      {
        http = "https://api.hitbtc.com/api/2";
        ws   = "wss://api.hitbtc.com/api/2/ws";
      };
      const json handshake() override {
        randId = Random::uuid32Id;
        symbol = base + quote;
        const json reply = Curl::xfer(http + "/public/symbol/" + symbol);
        minTick = stod(reply.value("tickSize", "0"));
        minSize = stod(reply.value("quantityIncrement", "0"));
        base    = reply.value("baseCurrency", base);
        quote   = reply.value("quoteCurrency", quote);
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &auth, const string &post) {
        return Curl::perform(url, [&](CURL *curl) {
          curl_easy_setopt(curl, CURLOPT_USERPWD, auth.data());
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwCoinbase: public GwApiWS,
                    public FIX::NullApplication {
    public:
      GwCoinbase()
      {
        http = "https://api.pro.coinbase.com";
        ws   = "wss://ws-feed.pro.coinbase.com";
        fix  = "fix.pro.coinbase.com:4198";
      };
      const json handshake() override {
        randId = Random::uuid36Id;
        symbol = base + "-" + quote;
        const json reply = Curl::xfer(http + "/products/" + symbol);
        minTick = stod(reply.value("quote_increment", "0"));
        minSize = stod(reply.value("base_min_size", "0"));
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &h4, const bool &rm) {
        return Curl::perform(url, [&](CURL *curl) {
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
        http = "https://api.bitfinex.com/v1";
        ws   = "wss://api.bitfinex.com/ws/2";
        askForReplace = true;
      };
      const json handshake() override {
        randId = Random::int45Id;
        symbol = Text::strL(base + quote);
        const json reply1 = Curl::xfer(http + "/pubticker/" + symbol);
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
        const json reply2 = Curl::xfer(http + "/symbols_details");
        if (reply2.is_array())
          for (const json &it : reply2)
            if (it.find("pair") != it.end() and it.value("pair", "") == symbol) {
              minSize = stod(it.value("minimum_order_size", "0"));
              break;
            }
        return { reply1, reply2 };
      };
    protected:
      static const json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::perform(url, [&](CURL *curl) {
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
        http = "https://api.fcoin.com/v2/";
        ws   = "wss://api.fcoin.com/v2/ws";
      };
      const json handshake() override {
        randId = Random::char16Id;
        symbol = Text::strL(base + quote);
        const json reply = Curl::xfer(http + "public/symbols");
        if (reply.find("data") != reply.end() and reply.at("data").is_array())
          for (const json &it : reply.at("data"))
            if (it.find("name") != it.end() and it.value("name", "") == symbol) {
              istringstream iss(
                "1e-" + to_string(it.value("price_decimal", 0))
                + " 1e-" + to_string(it.value("amount_decimal", 0))
              );
              iss >> minTick >> minSize;
              break;
            }
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &h3, const string &post = "") {
        return Curl::perform(url, [&](CURL *curl) {
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
        http = "https://api.kraken.com";
      };
      const json handshake() override {
        randId = Random::int32Id;
        symbol = base + quote;
        const json reply = Curl::xfer(http + "/0/public/AssetPairs?pair=" + symbol);
        if (reply.find("result") != reply.end())
          for (json::const_iterator it = reply.at("result").cbegin(); it != reply.at("result").cend(); ++it)
            if (it.value().find("pair_decimals") != it.value().end()) {
              istringstream iss(
                "1e-" + to_string(it.value().value("pair_decimals", 0))
                + " 1e-" + to_string(it.value().value("lot_decimals", 0))
              );
              iss >> minTick >> minSize;
              symbol = it.key();
              base = it.value().value("base", base);
              quote = it.value().value("quote", quote);
              break;
            }
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &h2, const string &post) {
        return Curl::perform(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, ("API-Key: " + h1).data());
          h_ = curl_slist_append(h_, ("API-Sign: " + h2).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
  };
  class GwKorbit: public GwApiREST {
    public:
      GwKorbit()
      {
        http = "https://api.korbit.co.kr/v1";
      };
      const json handshake() override {
        randId = Random::int45Id;
        symbol = Text::strL(base + "_" + quote);
        const json reply = Curl::xfer(http + "/constants");
        if (reply.find(symbol.substr(0,3).append("TickSize")) != reply.end()) {
          minTick = reply.value(symbol.substr(0,3).append("TickSize"), 0.0);
          minSize = 0.015;
        }
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &h1, const string &post) {
        return Curl::perform(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          if (!post.empty()) {
            h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
          }
          h_ = curl_slist_append(h_, ("Authorization: Bearer " + h1).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        });
      };
  };
  class GwPoloniex: public GwApiREST {
    public:
      GwPoloniex()
      {
        http = "https://poloniex.com";
      };
      const json handshake() override {
        randId = Random::int45Id;
        symbol = quote + "_" + base;
        const json reply = Curl::xfer(http + "/public?command=returnTicker");
        if (reply.find(symbol) != reply.end()) {
          istringstream iss("1e-" + to_string(6-reply.at(symbol).at("last").get<string>().find(".")));
          iss >> minTick;
          minSize = 0.001;
        }
        return reply;
      };
    protected:
      static const json xfer(const string &url, const string &post, const string &h1, const string &h2) {
        return Curl::perform(url, [&](CURL *curl) {
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

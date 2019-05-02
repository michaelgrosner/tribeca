#ifndef K_DATA_H_
#define K_DATA_H_
//! \file
//! \brief Internal data objects.

namespace ₿ {
  enum class mQuotingMode: unsigned int {
    Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth
  };
  enum class mQuotingSafety: unsigned int {
    Off, PingPong, PingPoing, Boomerang, AK47
  };
  enum class mQuoteState: unsigned int {
    Disconnected,  Live,             DisabledQuotes,
    MissingData,   UnknownHeld,      WidthMustBeSmaller,
    TBPHeld,       MaxTradesSeconds, WaitingPing,
    DepletedFunds, Crossed,
    UpTrendHeld,   DownTrendHeld
  };
  enum class mFairValueModel: unsigned int {
    BBO, wBBO, rwBBO
  };
  enum class mAutoPositionMode: unsigned int {
    Manual, EWMA_LS, EWMA_LMS, EWMA_4
  };
  enum class mPDivMode: unsigned int {
    Manual, Linear, Sine, SQRT, Switch
  };
  enum class mAPR: unsigned int {
    Off, Size, SizeWidth
  };
  enum class mSideAPR: unsigned int {
    Off, Buy, Sell
  };
  enum class mSOP: unsigned int {
    Off, Trades, Size, TradesSize
  };
  enum class mSTDEV: unsigned int {
    Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff
  };
  enum class mPingAt: unsigned int {
    BothSides,    BidSide,         AskSide,
    DepletedSide, DepletedBidSide, DepletedAskSide,
    StopPings
  };
  enum class mPongAt: unsigned int {
    ShortPingFair,       AveragePingFair,       LongPingFair,
    ShortPingAggressive, AveragePingAggressive, LongPingAggressive
  };

  struct mQuotingParams: public Sqlite::StructBackup<mQuotingParams>,
                         public Client::Broadcast<mQuotingParams>,
                         public Client::Clickable {
    Price             widthPing                       = 300.0;
    double            widthPingPercentage             = 0.25;
    Price             widthPong                       = 300.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    Amount            bestWidthSize                   = 0;
    Amount            buySize                         = 0.02;
    double            buySizePercentage               = 7.0;
    bool              buySizeMax                      = false;
    Amount            sellSize                        = 0.01;
    double            sellSizePercentage              = 7.0;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::PingPong;
    unsigned int      bullets                         = 2;
    Price             range                           = 0.5;
    double            rangePercentage                 = 5.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    Amount            targetBasePosition              = 1.0;
    double            targetBasePositionPercentage    = 50.0;
    Amount            positionDivergence              = 0.9;
    Amount            positionDivergenceMin           = 0.4;
    double            positionDivergencePercentage    = 21.0;
    double            positionDivergencePercentageMin = 10.0;
    mPDivMode         positionDivergenceMode          = mPDivMode::Manual;
    bool              percentageValues                = false;
    mAutoPositionMode autoPositionMode                = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing   = mAPR::Off;
    mSOP              superTrades                     = mSOP::Off;
    double            tradesPerMinute                 = 0.9;
    unsigned int      tradeRateSeconds                = 3;
    bool              protectionEwmaWidthPing         = false;
    bool              protectionEwmaQuotePrice        = true;
    unsigned int      protectionEwmaPeriods           = 200;
    mSTDEV            quotingStdevProtection          = mSTDEV::Off;
    bool              quotingStdevBollingerBands      = false;
    double            quotingStdevProtectionFactor    = 1.0;
    unsigned int      quotingStdevProtectionPeriods   = 1200;
    double            ewmaSensiblityPercentage        = 0.5;
    bool              quotingEwmaTrendProtection      = false;
    double            quotingEwmaTrendThreshold       = 2.0;
    unsigned int      veryLongEwmaPeriods             = 400;
    unsigned int      longEwmaPeriods                 = 200;
    unsigned int      mediumEwmaPeriods               = 100;
    unsigned int      shortEwmaPeriods                = 50;
    unsigned int      extraShortEwmaPeriods           = 12;
    unsigned int      ultraShortEwmaPeriods           = 3;
    double            aprMultiplier                   = 2;
    double            sopWidthMultiplier              = 2;
    double            sopSizeMultiplier               = 2;
    double            sopTradesMultiplier             = 2;
    bool              cancelOrdersAuto                = false;
    double            cleanPongsAuto                  = 0.0;
    double            profitHourInterval              = 0.5;
    bool              audio                           = false;
    unsigned int      delayUI                         = 3;
    int               _diffEwma                       = -1;
    private_ref:
      const KryptoNinja &K;
    public:
      mQuotingParams(const KryptoNinja &bot)
        : StructBackup(bot)
        , Broadcast(bot)
        , Clickable(bot)
        , K(bot)
      {};
      void from_json(const json &j) {
        const vector<unsigned int> previous = {
          veryLongEwmaPeriods,
          longEwmaPeriods,
          mediumEwmaPeriods,
          shortEwmaPeriods,
          extraShortEwmaPeriods,
          ultraShortEwmaPeriods
        };
        widthPing                       = fmax(K.gateway->minTick, j.value("widthPing", widthPing));
        widthPingPercentage             = fmin(1e+2, fmax(1e-3,    j.value("widthPingPercentage", widthPingPercentage)));
        widthPong                       = fmax(K.gateway->minTick, j.value("widthPong", widthPong));
        widthPongPercentage             = fmin(1e+2, fmax(1e-3,    j.value("widthPongPercentage", widthPongPercentage)));
        widthPercentage                 =                          j.value("widthPercentage", widthPercentage);
        bestWidth                       =                          j.value("bestWidth", bestWidth);
        bestWidthSize                   = fmax(0,                  j.value("bestWidthSize", bestWidthSize));
        buySize                         = fmax(K.gateway->minSize, j.value("buySize", buySize));
        buySizePercentage               = fmin(1e+2, fmax(1e-3,    j.value("buySizePercentage", buySizePercentage)));
        buySizeMax                      =                          j.value("buySizeMax", buySizeMax);
        sellSize                        = fmax(K.gateway->minSize, j.value("sellSize", sellSize));
        sellSizePercentage              = fmin(1e+2, fmax(1e-3,    j.value("sellSizePercentage", sellSizePercentage)));
        sellSizeMax                     =                          j.value("sellSizeMax", sellSizeMax);
        pingAt                          =                          j.value("pingAt", pingAt);
        pongAt                          =                          j.value("pongAt", pongAt);
        mode                            =                          j.value("mode", mode);
        safety                          =                          j.value("safety", safety);
        bullets                         = fmin(10, fmax(1,         j.value("bullets", bullets)));
        range                           =                          j.value("range", range);
        rangePercentage                 = fmin(1e+2, fmax(1e-3,    j.value("rangePercentage", rangePercentage)));
        fvModel                         =                          j.value("fvModel", fvModel);
        targetBasePosition              =                          j.value("targetBasePosition", targetBasePosition);
        targetBasePositionPercentage    = fmin(1e+2, fmax(0,       j.value("targetBasePositionPercentage", targetBasePositionPercentage)));
        positionDivergenceMin           =                          j.value("positionDivergenceMin", positionDivergenceMin);
        positionDivergenceMode          =                          j.value("positionDivergenceMode", positionDivergenceMode);
        positionDivergence              =                          j.value("positionDivergence", positionDivergence);
        positionDivergencePercentage    = fmin(1e+2, fmax(0,       j.value("positionDivergencePercentage", positionDivergencePercentage)));
        positionDivergencePercentageMin = fmin(1e+2, fmax(0,       j.value("positionDivergencePercentageMin", positionDivergencePercentageMin)));
        percentageValues                =                          j.value("percentageValues", percentageValues);
        autoPositionMode                =                          j.value("autoPositionMode", autoPositionMode);
        aggressivePositionRebalancing   =                          j.value("aggressivePositionRebalancing", aggressivePositionRebalancing);
        superTrades                     =                          j.value("superTrades", superTrades);
        tradesPerMinute                 =                          j.value("tradesPerMinute", tradesPerMinute);
        tradeRateSeconds                = fmax(0,                  j.value("tradeRateSeconds", tradeRateSeconds));
        protectionEwmaWidthPing         =                          j.value("protectionEwmaWidthPing", protectionEwmaWidthPing);
        protectionEwmaQuotePrice        =                          j.value("protectionEwmaQuotePrice", protectionEwmaQuotePrice);
        protectionEwmaPeriods           = fmax(1,                  j.value("protectionEwmaPeriods", protectionEwmaPeriods));
        quotingStdevProtection          =                          j.value("quotingStdevProtection", quotingStdevProtection);
        quotingStdevBollingerBands      =                          j.value("quotingStdevBollingerBands", quotingStdevBollingerBands);
        quotingStdevProtectionFactor    =                          j.value("quotingStdevProtectionFactor", quotingStdevProtectionFactor);
        quotingStdevProtectionPeriods   = fmax(1,                  j.value("quotingStdevProtectionPeriods", quotingStdevProtectionPeriods));
        ewmaSensiblityPercentage        =                          j.value("ewmaSensiblityPercentage", ewmaSensiblityPercentage);
        quotingEwmaTrendProtection      =                          j.value("quotingEwmaTrendProtection", quotingEwmaTrendProtection);
        quotingEwmaTrendThreshold       =                          j.value("quotingEwmaTrendThreshold", quotingEwmaTrendThreshold);
        veryLongEwmaPeriods             = fmax(1,                  j.value("veryLongEwmaPeriods", veryLongEwmaPeriods));
        longEwmaPeriods                 = fmax(1,                  j.value("longEwmaPeriods", longEwmaPeriods));
        mediumEwmaPeriods               = fmax(1,                  j.value("mediumEwmaPeriods", mediumEwmaPeriods));
        shortEwmaPeriods                = fmax(1,                  j.value("shortEwmaPeriods", shortEwmaPeriods));
        extraShortEwmaPeriods           = fmax(1,                  j.value("extraShortEwmaPeriods", extraShortEwmaPeriods));
        ultraShortEwmaPeriods           = fmax(1,                  j.value("ultraShortEwmaPeriods", ultraShortEwmaPeriods));
        aprMultiplier                   =                          j.value("aprMultiplier", aprMultiplier);
        sopWidthMultiplier              =                          j.value("sopWidthMultiplier", sopWidthMultiplier);
        sopSizeMultiplier               =                          j.value("sopSizeMultiplier", sopSizeMultiplier);
        sopTradesMultiplier             =                          j.value("sopTradesMultiplier", sopTradesMultiplier);
        cancelOrdersAuto                =                          j.value("cancelOrdersAuto", cancelOrdersAuto);
        cleanPongsAuto                  =                          j.value("cleanPongsAuto", cleanPongsAuto);
        profitHourInterval              =                          j.value("profitHourInterval", profitHourInterval);
        audio                           =                          j.value("audio", audio);
        delayUI                         = fmax(0,                  j.value("delayUI", delayUI));
        if (mode == mQuotingMode::Depth)
          widthPercentage = false;
        K.gateway->askForCancelAll = cancelOrdersAuto;
        K.timer_ticks_factor(delayUI);
        K.client_queue_delay(delayUI);
        if (_diffEwma == -1) _diffEwma++;
        else {
          _diffEwma |= (previous[0] != veryLongEwmaPeriods)   << 0;
          _diffEwma |= (previous[1] != longEwmaPeriods)       << 1;
          _diffEwma |= (previous[2] != mediumEwmaPeriods)     << 2;
          _diffEwma |= (previous[3] != shortEwmaPeriods)      << 3;
          _diffEwma |= (previous[4] != extraShortEwmaPeriods) << 4;
          _diffEwma |= (previous[5] != ultraShortEwmaPeriods) << 5;
        }
        K.clicked(this);
        _diffEwma = 0;
      };
      void click(const json &j) override {
        from_json(j);
        backup();
        broadcast();
      };
      const mMatter about() const override {
        return mMatter::QuotingParameters;
      };
    private:
      const string explain() const override {
        return "Quoting Parameters";
      };
      string explainKO() const override {
        return "using default values for %";
      };
  };
  static void to_json(json &j, const mQuotingParams &k) {
    j = {
      {                      "widthPing", k.widthPing                      },
      {            "widthPingPercentage", k.widthPingPercentage            },
      {                      "widthPong", k.widthPong                      },
      {            "widthPongPercentage", k.widthPongPercentage            },
      {                "widthPercentage", k.widthPercentage                },
      {                      "bestWidth", k.bestWidth                      },
      {                  "bestWidthSize", k.bestWidthSize                  },
      {                        "buySize", k.buySize                        },
      {              "buySizePercentage", k.buySizePercentage              },
      {                     "buySizeMax", k.buySizeMax                     },
      {                       "sellSize", k.sellSize                       },
      {             "sellSizePercentage", k.sellSizePercentage             },
      {                    "sellSizeMax", k.sellSizeMax                    },
      {                         "pingAt", k.pingAt                         },
      {                         "pongAt", k.pongAt                         },
      {                           "mode", k.mode                           },
      {                         "safety", k.safety                         },
      {                        "bullets", k.bullets                        },
      {                          "range", k.range                          },
      {                "rangePercentage", k.rangePercentage                },
      {                        "fvModel", k.fvModel                        },
      {             "targetBasePosition", k.targetBasePosition             },
      {   "targetBasePositionPercentage", k.targetBasePositionPercentage   },
      {             "positionDivergence", k.positionDivergence             },
      {   "positionDivergencePercentage", k.positionDivergencePercentage   },
      {          "positionDivergenceMin", k.positionDivergenceMin          },
      {"positionDivergencePercentageMin", k.positionDivergencePercentageMin},
      {         "positionDivergenceMode", k.positionDivergenceMode         },
      {               "percentageValues", k.percentageValues               },
      {               "autoPositionMode", k.autoPositionMode               },
      {  "aggressivePositionRebalancing", k.aggressivePositionRebalancing  },
      {                    "superTrades", k.superTrades                    },
      {                "tradesPerMinute", k.tradesPerMinute                },
      {               "tradeRateSeconds", k.tradeRateSeconds               },
      {        "protectionEwmaWidthPing", k.protectionEwmaWidthPing        },
      {       "protectionEwmaQuotePrice", k.protectionEwmaQuotePrice       },
      {          "protectionEwmaPeriods", k.protectionEwmaPeriods          },
      {         "quotingStdevProtection", k.quotingStdevProtection         },
      {     "quotingStdevBollingerBands", k.quotingStdevBollingerBands     },
      {   "quotingStdevProtectionFactor", k.quotingStdevProtectionFactor   },
      {  "quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods  },
      {       "ewmaSensiblityPercentage", k.ewmaSensiblityPercentage       },
      {     "quotingEwmaTrendProtection", k.quotingEwmaTrendProtection     },
      {      "quotingEwmaTrendThreshold", k.quotingEwmaTrendThreshold      },
      {            "veryLongEwmaPeriods", k.veryLongEwmaPeriods            },
      {                "longEwmaPeriods", k.longEwmaPeriods                },
      {              "mediumEwmaPeriods", k.mediumEwmaPeriods              },
      {               "shortEwmaPeriods", k.shortEwmaPeriods               },
      {          "extraShortEwmaPeriods", k.extraShortEwmaPeriods          },
      {          "ultraShortEwmaPeriods", k.ultraShortEwmaPeriods          },
      {                  "aprMultiplier", k.aprMultiplier                  },
      {             "sopWidthMultiplier", k.sopWidthMultiplier             },
      {              "sopSizeMultiplier", k.sopSizeMultiplier              },
      {            "sopTradesMultiplier", k.sopTradesMultiplier            },
      {               "cancelOrdersAuto", k.cancelOrdersAuto               },
      {                 "cleanPongsAuto", k.cleanPongsAuto                 },
      {             "profitHourInterval", k.profitHourInterval             },
      {                          "audio", k.audio                          },
      {                        "delayUI", k.delayUI                        }
    };
  };
  static void from_json(const json &j, mQuotingParams &k) {
    k.from_json(j);
  };

  struct mLastOrder {
    Price  price;
    Amount tradeQuantity;
    Side   side;
    bool   isPong;
  };
  struct mOrders: public Client::Broadcast<mOrders> {
    mLastOrder updated;
    private:
      unordered_map<RandId, mOrder> orders;
    private_ref:
      const KryptoNinja &K;
    public:
      mOrders(const KryptoNinja &bot)
        : Broadcast(bot)
        , updated()
        , K(bot)
      {};
      mOrder *const find(const RandId &orderId) {
        return (orderId.empty()
          or orders.find(orderId) == orders.end()
        ) ? nullptr
          : &orders.at(orderId);
      };
      mOrder *const findsert(const mOrder &raw) {
        if (raw.status == Status::Waiting and !raw.orderId.empty())
          return &(orders[raw.orderId] = raw);
        if (raw.orderId.empty() and !raw.exchangeId.empty()) {
          auto it = find_if(
            orders.begin(), orders.end(),
            [&](const pair<RandId, mOrder> &it_) {
              return raw.exchangeId == it_.second.exchangeId;
            }
          );
          if (it != orders.end())
            return &it->second;
        }
        return find(raw.orderId);
      };
      const Amount heldAmount(const Side &side) const {
        Amount held = 0;
        for (const auto &it : orders)
          if (it.second.side == side)
            held += (
              it.second.side == Side::Ask
                ? it.second.quantity
                : it.second.quantity * it.second.price
            );
        return held;
      };
      void resetFilters(
        unordered_map<Price, Amount> *const filterBidOrders,
        unordered_map<Price, Amount> *const filterAskOrders
      ) const {
        filterBidOrders->clear();
        filterAskOrders->clear();
        for (const auto &it : orders)
          (it.second.side == Side::Bid
            ? *filterBidOrders
            : *filterAskOrders
          )[it.second.price] += it.second.quantity;
      };
      const vector<mOrder*> at(const Side &side) {
        vector<mOrder*> sideOrders;
        for (auto &it : orders)
          if (side == it.second.side)
             sideOrders.push_back(&it.second);
        return sideOrders;
      };
      const vector<mOrder*> working() {
        vector<mOrder*> workingOrders;
        for (auto &it : orders)
          if (Status::Working == it.second.status
            and !it.second.disablePostOnly
          ) workingOrders.push_back(&it.second);
        return workingOrders;
      };
      const vector<mOrder> working(const bool &sorted = false) const {
        vector<mOrder> workingOrders;
        for (const auto &it : orders)
          if (Status::Working == it.second.status)
            workingOrders.push_back(it.second);
        if (sorted)
          sort(workingOrders.begin(), workingOrders.end(),
            [](const mOrder &a, const mOrder &b) {
              return a.price > b.price;
            }
          );
        return workingOrders;
      };
      mOrder *const upsert(const mOrder &raw) {
        mOrder *const order = findsert(raw);
        mOrder::update(raw, order);
        if (K.arg<int>("debug-orders")) {
          report(order, " saved ");
          report_size();
        }
        return order;
      };
      const bool replace(const Price &price, const bool &isPong, mOrder *const order) {
        const bool allowed = mOrder::replace(price, isPong, order);
        if (K.arg<int>("debug-orders")) report(order, "replace");
        return allowed;
      };
      const bool cancel(mOrder *const order) {
        const bool allowed = mOrder::cancel(order);
        if (K.arg<int>("debug-orders")) report(order, "cancel ");
        return allowed;
      };
      void purge(const mOrder *const order) {
        if (K.arg<int>("debug-orders")) report(order, " purge ");
        orders.erase(order->orderId);
        if (K.arg<int>("debug-orders")) report_size();
      };
      void read_from_gw(const mOrder &raw) {
        if (K.arg<int>("debug-orders")) report(&raw, " reply ");
        mOrder *const order = upsert(raw);
        if (!order) {
          updated = {};
          return;
        }
        updated = {
          order->price,
          raw.tradeQuantity,
          order->side,
          order->isPong
        };
        if (order->status == Status::Terminated)
          purge(order);
        broadcast();
        Print::repaint();
      };
      const mMatter about() const override {
        return mMatter::OrderStatusReports;
      };
      const bool realtime() const override {
        return false;
      };
      const json blob() const override {
        return working();
      };
    private:
      void report(const mOrder *const order, const string &reason) const {
        Print::log("DEBUG OG", " " + reason + " " + (
          order
            ? order->orderId + "::" + order->exchangeId
              + " [" + to_string((int)order->status) + "]: "
              + K.gateway->decimal.amount.str(order->quantity) + " " + K.gateway->base + " at price "
              + K.gateway->decimal.price.str(order->price) + " " + K.gateway->quote
            : "not found"
        ));
      };
      void report_size() const {
        Print::log("DEBUG OG", "memory " + to_string(orders.size()));
      };
  };
  static void to_json(json &j, const mOrders &k) {
    j = k.blob();
  };

  struct mMarketTakers: public Client::Broadcast<mTrade> {
    public:
      vector<mTrade> trades;
              Amount takersBuySize60s  = 0,
                     takersSellSize60s = 0;
      mMarketTakers(const KryptoNinja &bot)
        : Broadcast(bot)
      {};
      void timer_60s() {
        takersSellSize60s = takersBuySize60s = 0;
        for (const auto &it : trades)
          (it.side == Side::Bid
            ? takersSellSize60s
            : takersBuySize60s
          ) += it.quantity;
        trades.clear();
      };
      void read_from_gw(const mTrade &raw) {
        trades.push_back(raw);
        broadcast();
      };
      const mMatter about() const override {
        return mMatter::MarketTrade;
      };
      const json blob() const override {
        return trades.back();
      };
      const json hello() override {
        return trades;
      };
  };
  static void to_json(json &j, const mMarketTakers &k) {
    j = k.trades;
  };

  struct mFairLevelsPrice: public Client::Broadcast<mFairLevelsPrice> {
    private_ref:
      const Price &fairValue;
    public:
      mFairLevelsPrice(const KryptoNinja &bot, const Price &f)
        : Broadcast(bot)
        , fairValue(f)
      {};
      const Price currentPrice() const {
        return fairValue;
      };
      const mMatter about() const override {
        return mMatter::FairValue;
      };
      const bool realtime() const override {
        return false;
      };
      const bool read_same_blob() const override {
        return false;
      };
      const bool read_asap() const override {
        return false;
      };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", k.currentPrice()}
    };
  };

  struct mStdev {
    Price fv,
          topBid,
          topAsk;
  };
  static void to_json(json &j, const mStdev &k) {
    j = {
      { "fv", k.fv    },
      {"bid", k.topBid},
      {"ask", k.topAsk}
    };
  };
  static void from_json(const json &j, mStdev &k) {
    k.fv     = j.value("fv",  0.0);
    k.topBid = j.value("bid", 0.0);
    k.topAsk = j.value("ask", 0.0);
  };

  struct mStdevs: public Sqlite::VectorBackup<mStdev> {
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    private_ref:
      const Price          &fairValue;
      const mQuotingParams &qp;
    public:
      mStdevs(const KryptoNinja &bot, const Price &f, const mQuotingParams &q)
        : VectorBackup(bot)
        , fairValue(f)
        , qp(q)
      {};
      void timer_1s(const Price &topBid, const Price &topAsk) {
        push_back({fairValue, topBid, topAsk});
        calc();
      };
      void calc() {
        if (size() < 2) return;
        fair = calc(&fairMean, "fv");
        bid  = calc(&bidMean, "bid");
        ask  = calc(&askMean, "ask");
        top  = calc(&topMean, "top");
      };
      const mMatter about() const override {
        return mMatter::STDEVStats;
      };
      const double limit() const override {
        return qp.quotingStdevProtectionPeriods;
      };
      const Clock lifetime() const override {
        return 1e+3 * limit();
      };
    private:
      double calc(Price *const mean, const string &type) const {
        vector<Price> values;
        for (const mStdev &it : rows)
          if (type == "fv")
            values.push_back(it.fv);
          else if (type == "bid")
            values.push_back(it.topBid);
          else if (type == "ask")
            values.push_back(it.topAsk);
          else if (type == "top") {
            values.push_back(it.topBid);
            values.push_back(it.topAsk);
          }
        return calc(mean, qp.quotingStdevProtectionFactor, values);
      };
      double calc(Price *const mean, const double &factor, const vector<Price> &values) const {
        unsigned int n = values.size();
        if (!n) return 0;
        double sum = 0;
        for (const Price &it : values) sum += it;
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (const Price &it : values) {
          Price diff = it - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * factor;
      };
      string explainOK() const override {
        return "loaded % STDEV Periods";
      };
  };
  static void to_json(json &j, const mStdevs &k) {
    j = {
      {      "fv", k.fair    },
      {  "fvMean", k.fairMean},
      {    "tops", k.top     },
      {"topsMean", k.topMean },
      {     "bid", k.bid     },
      { "bidMean", k.bidMean },
      {     "ask", k.ask     },
      { "askMean", k.askMean }
    };
  };

  struct mFairHistory: public Sqlite::VectorBackup<Price> {
    public:
      mFairHistory(const KryptoNinja &bot)
        : VectorBackup(bot)
      {};
      const mMatter about() const override {
        return mMatter::MarketDataLongTerm;
      };
      const double limit() const override {
        return 5760;
      };
      const Clock lifetime() const override {
        return 60e+3 * limit();
      };
    private:
      string explainOK() const override {
        return "loaded % historical Fair Values";
      };
  };

  struct mEwma: public Sqlite::StructBackup<mEwma>,
                public Client::Clicked::Catch {
    mFairHistory fairValue96h;
           Price mgEwmaVL = 0,
                 mgEwmaL  = 0,
                 mgEwmaM  = 0,
                 mgEwmaS  = 0,
                 mgEwmaXS = 0,
                 mgEwmaU  = 0,
                 mgEwmaP  = 0,
                 mgEwmaW  = 0;
          double mgEwmaTrendDiff              = 0,
                 targetPositionAutoPercentage = 0;
    private_ref:
      const Price          &fairValue;
      const mQuotingParams &qp;
    public:
      mEwma(const KryptoNinja &bot, const Price &f, const mQuotingParams &q)
        : StructBackup(bot)
        , Catch(bot, {
            {&q, [&]() { calcFromHistory(); }}
          })
        , fairValue96h(bot)
        , fairValue(f)
        , qp(q)
      {};
      void timer_60s(const Price &averageWidth) {
        prepareHistory();
        calcProtections(averageWidth);
        calcPositions();
        calcTargetPositionAutoPercentage();
        backup();
      };
      const mMatter about() const override {
        return mMatter::EWMAStats;
      };
      const Clock lifetime() const override {
        return 60e+3 * max(qp.veryLongEwmaPeriods,
                       max(qp.longEwmaPeriods,
                       max(qp.mediumEwmaPeriods,
                       max(qp.shortEwmaPeriods,
                       max(qp.extraShortEwmaPeriods,
                           qp.ultraShortEwmaPeriods
                       )))));
      };
    private:
      void calcFromHistory() {
        if ((qp._diffEwma >> 0) & 1) calcFromHistory(&mgEwmaVL, qp.veryLongEwmaPeriods,   "VeryLong");
        if ((qp._diffEwma >> 1) & 1) calcFromHistory(&mgEwmaL,  qp.longEwmaPeriods,       "Long");
        if ((qp._diffEwma >> 2) & 1) calcFromHistory(&mgEwmaM,  qp.mediumEwmaPeriods,     "Medium");
        if ((qp._diffEwma >> 3) & 1) calcFromHistory(&mgEwmaS,  qp.shortEwmaPeriods,      "Short");
        if ((qp._diffEwma >> 4) & 1) calcFromHistory(&mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
        if ((qp._diffEwma >> 5) & 1) calcFromHistory(&mgEwmaU,  qp.ultraShortEwmaPeriods, "UltraShort");
      };
      void calc(Price *const mean, const unsigned int &periods, const Price &value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void prepareHistory() {
        fairValue96h.push_back(fairValue);
      };
      void calcFromHistory(Price *const mean, const unsigned int &periods, const string &name) {
        unsigned int n = fairValue96h.size();
        if (!n--) return;
        unsigned int x = 0;
        *mean = fairValue96h.front();
        while (n--) calc(mean, periods, fairValue96h.at(++x));
        Print::log("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
      };
      void calcPositions() {
        calc(&mgEwmaVL, qp.veryLongEwmaPeriods,   fairValue);
        calc(&mgEwmaL,  qp.longEwmaPeriods,       fairValue);
        calc(&mgEwmaM,  qp.mediumEwmaPeriods,     fairValue);
        calc(&mgEwmaS,  qp.shortEwmaPeriods,      fairValue);
        calc(&mgEwmaXS, qp.extraShortEwmaPeriods, fairValue);
        calc(&mgEwmaU,  qp.ultraShortEwmaPeriods, fairValue);
        if (mgEwmaXS and mgEwmaU)
          mgEwmaTrendDiff = ((mgEwmaU * 1e+2) / mgEwmaXS) - 1e+2;
      };
      void calcProtections(const Price &averageWidth) {
        calc(&mgEwmaP, qp.protectionEwmaPeriods, fairValue);
        calc(&mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
      };
      void calcTargetPositionAutoPercentage() {
        unsigned int max3size = min((size_t)3, fairValue96h.size());
        Price SMA3 = accumulate(fairValue96h.end() - max3size, fairValue96h.end(), Price(),
          [](Price sma3, const Price &it) { return sma3 + it; }
        ) / max3size;
        double targetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 1e+2 / mgEwmaL) - 1e+2);
          double newEwmacrossing = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2);
          targetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          targetPosition = ((mgEwmaS * 1e+2 / mgEwmaL) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (mgEwmaL < mgEwmaVL) targetPosition = -1;
          else targetPosition = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        }
        targetPositionAutoPercentage = ((1 + max(-1.0, min(1.0, targetPosition))) / 2) * 1e+2;
      };
      const string explain() const override {
        return "EWMA Values";
      };
      string explainKO() const override {
        return "consider to warm up some %";
      };
  };
  static void to_json(json &j, const mEwma &k) {
    j = {
      {  "ewmaVeryLong", k.mgEwmaVL       },
      {      "ewmaLong", k.mgEwmaL        },
      {    "ewmaMedium", k.mgEwmaM        },
      {     "ewmaShort", k.mgEwmaS        },
      {"ewmaExtraShort", k.mgEwmaXS       },
      {"ewmaUltraShort", k.mgEwmaU        },
      {     "ewmaQuote", k.mgEwmaP        },
      {     "ewmaWidth", k.mgEwmaW        },
      { "ewmaTrendDiff", k.mgEwmaTrendDiff}
    };
  };
  static void from_json(const json &j, mEwma &k) {
    k.mgEwmaVL = j.value("ewmaVeryLong",   0.0);
    k.mgEwmaL  = j.value("ewmaLong",       0.0);
    k.mgEwmaM  = j.value("ewmaMedium",     0.0);
    k.mgEwmaS  = j.value("ewmaShort",      0.0);
    k.mgEwmaXS = j.value("ewmaExtraShort", 0.0);
    k.mgEwmaU  = j.value("ewmaUltraShort", 0.0);
  };

  struct mMarketStats: public Client::Broadcast<mMarketStats> {
               mEwma ewma;
             mStdevs stdev;
    mFairLevelsPrice fairPrice;
       mMarketTakers takerTrades;
    mMarketStats(const KryptoNinja &bot, const Price &f, const mQuotingParams &q)
      : Broadcast(bot)
      , ewma(bot, f, q)
      , stdev(bot, f, q)
      , fairPrice(bot, f)
      , takerTrades(bot)
    {};
    const mMatter about() const override {
      return mMatter::MarketChart;
    };
    const bool realtime() const override {
      return false;
    };
  };
  static void to_json(json &j, const mMarketStats &k) {
    j = {
      {          "ewma", k.ewma                         },
      {    "stdevWidth", k.stdev                        },
      {     "fairValue", k.fairPrice.currentPrice()     },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
    };
  };

  struct mLevelsDiff: public mLevels,
                      public Client::Broadcast<mLevelsDiff> {
      bool patched = false;
    private_ref:
      const mLevels        &unfiltered;
      const mQuotingParams &qp;
    public:
      mLevelsDiff(const KryptoNinja &bot, const mLevels &u, const mQuotingParams &q)
        : Broadcast(bot)
        , unfiltered(u)
        , qp(q)
      {};
      const bool empty() const {
        return patched
          ? bids.empty() and asks.empty()
          : bids.empty() or asks.empty();
      };
      void send_patch() {
        if (ratelimit()) return;
        diff();
        if (!empty() and read) read();
        unfilter();
      };
      const mMatter about() const override {
        return mMatter::MarketData;
      };
      const json hello() override {
        unfilter();
        return Broadcast::hello();
      };
    private:
      const bool ratelimit() {
        return unfiltered.bids.empty() or unfiltered.asks.empty() or empty()
          or !read_soon(qp.delayUI * 1e+3);
      };
      void unfilter() {
        bids = unfiltered.bids;
        asks = unfiltered.asks;
        patched = false;
      };
      void diff() {
        bids = diff(bids, unfiltered.bids);
        asks = diff(asks, unfiltered.asks);
        patched = true;
      };
      vector<mLevel> diff(const vector<mLevel> &from, vector<mLevel> to) const {
        vector<mLevel> patch;
        for (const mLevel &it : from) {
          auto it_ = find_if(
            to.begin(), to.end(),
            [&](const mLevel &_it) {
              return it.price == _it.price;
            }
          );
          Amount size = 0;
          if (it_ != to.end()) {
            size = it_->size;
            to.erase(it_);
          }
          if (size != it.size)
            patch.push_back({it.price, size});
        }
        if (!to.empty())
          patch.insert(patch.end(), to.begin(), to.end());
        return patch;
      };
  };
  static void to_json(json &j, const mLevelsDiff &k) {
    to_json(j, (mLevels)k);
    if (k.patched)
      j["diff"] = true;
  };
  struct mMarketLevels: public mLevels {
    unsigned int averageCount = 0;
           Price averageWidth = 0,
                 fairValue    = 0;
         mLevels unfiltered;
     mLevelsDiff diff;
    mMarketStats stats;
    private:
      unordered_map<Price, Amount> filterBidOrders,
                                   filterAskOrders;
    private_ref:
      const KryptoNinja    &K;
      const mQuotingParams &qp;
      const mOrders        &orders;
    public:
      mMarketLevels(const KryptoNinja &bot, const mQuotingParams &q, const mOrders &o)
        : diff(bot, unfiltered, q)
        , stats(bot, fairValue, q)
        , K(bot)
        , qp(q)
        , orders(o)
      {};
      const bool warn_empty() const {
        const bool err = bids.empty() or asks.empty();
        if (err and (float)clock()/CLOCKS_PER_SEC > 3.0)
          Print::logWar("QE", "Unable to calculate quote, missing market data");
        return err;
      };
      void timer_1s() {
        stats.stdev.timer_1s(bids.cbegin()->price, asks.cbegin()->price);
      };
      void timer_60s() {
        stats.takerTrades.timer_60s();
        stats.ewma.timer_60s(resetAverageWidth());
        stats.broadcast();
      };
      const Price calcQuotesWidth(bool *const superSpread) const {
        const Price widthPing = fmax(
          qp.widthPercentage
            ? qp.widthPingPercentage * fairValue / 100
            : qp.widthPing,
          qp.protectionEwmaWidthPing and stats.ewma.mgEwmaW
            ? stats.ewma.mgEwmaW
            : 0
        );
        *superSpread = asks.cbegin()->price - bids.cbegin()->price > widthPing * qp.sopWidthMultiplier;
        return widthPing;
      };
      void clear() {
        bids.clear();
        asks.clear();
      };
      const bool ready() {
        filter();
        return !(bids.empty() or asks.empty());
      };
      void read_from_gw(const mLevels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
        if (stats.fairPrice.broadcast()) Print::repaint();
        diff.send_patch();
      };
    private:
      void filter() {
        orders.resetFilters(&filterBidOrders, &filterAskOrders);
        bids = filter(unfiltered.bids, &filterBidOrders);
        asks = filter(unfiltered.asks, &filterAskOrders);
        calcFairValue();
        calcAverageWidth();
      };
      void calcAverageWidth() {
        if (bids.empty() or asks.empty()) return;
        averageWidth = (
          (averageWidth * averageCount)
            + asks.cbegin()->price
            - bids.cbegin()->price
        );
        averageWidth /= ++averageCount;
      };
      const Price resetAverageWidth() {
        averageCount = 0;
        return averageWidth;
      };
      void calcFairValue() {
        if (bids.empty() or asks.empty())
          fairValue = 0;
        else if (qp.fvModel == mFairValueModel::BBO)
          fairValue = (asks.cbegin()->price
                     + bids.cbegin()->price) / 2;
        else if (qp.fvModel == mFairValueModel::wBBO)
          fairValue = (
            bids.cbegin()->price * bids.cbegin()->size
          + asks.cbegin()->price * asks.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        else
          fairValue = (
            bids.cbegin()->price * asks.cbegin()->size
          + asks.cbegin()->price * bids.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        if (fairValue)
          fairValue = round(fairValue / K.gateway->minTick) * K.gateway->minTick;
      };
      const vector<mLevel> filter(vector<mLevel> levels, unordered_map<Price, Amount> *const filterOrders) {
        if (!filterOrders->empty())
          for (auto it = levels.begin(); it != levels.end();) {
            for (auto it_ = filterOrders->begin(); it_ != filterOrders->end();)
              if (abs(it->price - it_->first) < K.gateway->minTick) {
                it->size -= it_->second;
                filterOrders->erase(it_);
                break;
              } else ++it_;
            if (it->size < K.gateway->minSize) it = levels.erase(it);
            else ++it;
            if (filterOrders->empty()) break;
          }
        return levels;
      };
  };

  struct mProfit {
    Amount baseValue,
           quoteValue;
     Clock time;
  };
  static void to_json(json &j, const mProfit &k) {
    j = {
      { "baseValue", k.baseValue },
      {"quoteValue", k.quoteValue},
      {      "time", k.time      }
     };
  };
  static void from_json(const json &j, mProfit &k) {
    k.baseValue  = j.value("baseValue",  0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time",  (Clock)0);
  };
  struct mProfits: public Sqlite::VectorBackup<mProfit> {
    private_ref:
      const KryptoNinja    &K;
      const mQuotingParams &qp;
    public:
      mProfits(const KryptoNinja &bot, const mQuotingParams &q)
        : VectorBackup(bot)
        , K(bot)
        , qp(q)
      {};
      const bool ratelimit() const {
        return !empty() and crbegin()->time + 21e+3 > Tstamp;
      };
      const double calcBaseDiff() const {
        return calcDiffPercent(
          cbegin()->baseValue,
          crbegin()->baseValue
        );
      };
      const double calcQuoteDiff() const {
        return calcDiffPercent(
          cbegin()->quoteValue,
          crbegin()->quoteValue
        );
      };
      const double calcDiffPercent(Amount older, Amount newer) const {
        return K.gateway->decimal.percent.round(((newer - older) / newer) * 1e+2);
      };
      const mMatter about() const override {
        return mMatter::Profit;
      };
      void erase() override {
        const Clock now = Tstamp;
        for (auto it = begin(); it != end();)
          if (it->time + lifetime() > now) ++it;
          else it = rows.erase(it);
      };
      const double limit() const override {
        return qp.profitHourInterval;
      };
      const Clock lifetime() const override {
        return 3600e+3 * limit();
      };
    private:
      string explainOK() const override {
        return "loaded % historical Profits";
      };
  };

  struct mOrderFilled: public mTrade {
     string tradeId;
     Amount value,
            feeCharged,
            Kqty,
            Kvalue,
            Kdiff;
      Price Kprice;
      Clock Ktime;
       bool isPong,
            loadedFromDB;
  };
  static void to_json(json &j, const mOrderFilled &k) {
    j = {
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
  static void from_json(const json &j, mOrderFilled &k) {
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

  struct mButtonSubmitNewOrder: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonSubmitNewOrder(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if (j.is_object() and j.value("price", 0.0) and j.value("quantity", 0.0)) {
          json order = j;
          order["orderId"] = K.gateway->randId();
          K.clicked(this, order);
        }
      };
      const mMatter about() const override {
        return mMatter::SubmitNewOrder;
      };
  };
  struct mButtonCancelOrder: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonCancelOrder(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if ((j.is_object() and !j.value("orderId", "").empty()))
          K.clicked(this, j.at("orderId").get<RandId>());
      };
      const mMatter about() const override {
        return mMatter::CancelOrder;
      };
  };
  struct mButtonCancelAllOrders: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonCancelAllOrders(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        K.clicked(this);
      };
      const mMatter about() const override {
        return mMatter::CancelAllOrders;
      };
  };
  struct mButtonCleanAllClosedTrades: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonCleanAllClosedTrades(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        K.clicked(this);
      };
      const mMatter about() const override {
        return mMatter::CleanAllClosedTrades;
      };
  };
  struct mButtonCleanAllTrades: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonCleanAllTrades(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        K.clicked(this);
      };
      const mMatter about() const override {
        return mMatter::CleanAllTrades;
      };
  };
  struct mButtonCleanTrade: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      mButtonCleanTrade(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if ((j.is_object() and !j.value("tradeId", "").empty()))
          K.clicked(this, j.at("tradeId").get<string>());
      };
      const mMatter about() const override {
        return mMatter::CleanTrade;
      };
  };
  struct mNotepad: public Client::Broadcast<mNotepad>,
                   public Client::Clickable {
    public:
      string content;
    public:
      mNotepad(const KryptoNinja &bot)
        : Broadcast(bot)
        , Clickable(bot)
      {};
      void click(const json &j) override {
        if (j.is_array() and j.size() and j.at(0).is_string())
         content = j.at(0);
      };
      const mMatter about() const override {
        return mMatter::Notepad;
      };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mButtons {
    mNotepad                    notepad;
    mButtonSubmitNewOrder       submit;
    mButtonCancelOrder          cancel;
    mButtonCancelAllOrders      cancelAll;
    mButtonCleanAllClosedTrades cleanTradesClosed;
    mButtonCleanAllTrades       cleanTrades;
    mButtonCleanTrade           cleanTrade;
    mButtons(const KryptoNinja &bot)
      : notepad(bot)
      , submit(bot)
      , cancel(bot)
      , cancelAll(bot)
      , cleanTradesClosed(bot)
      , cleanTrades(bot)
      , cleanTrade(bot)
    {};
  };

  struct mTradesHistory: public Sqlite::VectorBackup<mOrderFilled>,
                         public Client::Broadcast<mOrderFilled>,
                         public Client::Clicked::Catch {
    private_ref:
      const KryptoNinja    &K;
      const mQuotingParams &qp;
    public:
      mTradesHistory(const KryptoNinja &bot, const mQuotingParams &q, const mButtons &b)
        : VectorBackup(bot)
        , Broadcast(bot)
        , Catch(bot, {
            {&b.cleanTrade, [&](const json &j) { clearOne(j); }},
            {&b.cleanTrades, [&]() { clearAll(); }},
            {&b.cleanTradesClosed, [&]() { clearClosed(); }}
          })
        , K(bot)
        , qp(q)
      {};
      void insert(const mLastOrder &order) {
        const Amount fee = 0;
        const Clock time = Tstamp;
        mOrderFilled filled = {
          order.side,
          order.price,
          order.tradeQuantity,
          time,
          to_string(time),
          abs(order.price * order.tradeQuantity),
          fee,
          0, 0, 0, 0, 0,
          order.isPong,
          false
        };
        Print::log("GW " + K.gateway->exchange, string(filled.isPong?"PONG":"PING") + " TRADE "
          + (filled.side == Side::Bid ? "BUY  " : "SELL ")
          + K.gateway->decimal.amount.str(filled.quantity) + ' ' + K.gateway->base + " at price "
          + K.gateway->decimal.price.str(filled.price) + ' ' + K.gateway->quote + " (value "
          + K.gateway->decimal.price.str(filled.value) + ' ' + K.gateway->quote + ")"
        );
        if (qp.safety == mQuotingSafety::Off or qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
          broadcast_push_back(filled);
        else {
          Price widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * filled.price / 100
            : qp.widthPong;
          map<Price, string> matches;
          for (mOrderFilled &it : rows) {
            if (it.quantity - it.Kqty > 0
              and it.side != filled.side
            ) {
              Price combinedFee = K.gateway->makeFee * (it.price + filled.price);
              if (filled.side == Side::Bid
                  ? (it.price > filled.price + widthPong + combinedFee)
                  : (it.price < filled.price - widthPong - combinedFee)
              ) matches[it.price] = it.tradeId;
            }
          }
          matchPong(
            matches,
            filled,
            (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive)
              ? filled.side == Side::Ask
              : filled.side == Side::Bid
          );
        }
        if (qp.cleanPongsAuto)
          clearPongsAuto();
      };
      const mMatter about() const override {
        return mMatter::Trades;
      };
      void erase() override {
        if (crbegin()->Kqty < 0) rows.pop_back();
      };
      const json blob() const override {
        if (crbegin()->Kqty == -1) return nullptr;
        else return VectorBackup::blob();
      };
      const string increment() const override {
        return crbegin()->tradeId;
      };
      const json hello() override {
        for (mOrderFilled &it : rows)
          it.loadedFromDB = true;
        return rows;
      };
    private:
      void clearAll() {
        clear_if([](iterator it) {
          return true;
        });
      };
      void clearOne(const string &tradeId) {
        clear_if([&tradeId](iterator it) {
          return it->tradeId == tradeId;
        }, true);
      };
      void clearClosed() {
        clear_if([](iterator it) {
          return it->Kqty >= it->quantity;
        });
      };
      void clearPongsAuto() {
        const Clock expire = Tstamp - (abs(qp.cleanPongsAuto) * 86400e3);
        const bool forcedClean = qp.cleanPongsAuto < 0;
        clear_if([&expire, &forcedClean](iterator it) {
          return (it->Ktime?:it->time) < expire and (
            forcedClean
            or it->Kqty >= it->quantity
          );
        });
      };
      void clear_if(const function<const bool(iterator)> &fn, const bool &onlyOne = false) {
        for (auto it = begin(); it != end();)
          if (fn(it)) {
            it->Kqty = -1;
            it = send_push_erase(it);
            if (onlyOne) break;
          } else ++it;
      };
      void matchPong(const map<Price, string> &matches, mOrderFilled pong, const bool &reverse) {
        if (reverse) for (auto it = matches.crbegin(); it != matches.crend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (const auto &it : matches)
          if (!matchPong(it.second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (auto it = begin(); it != end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->isPong = false;
            it->loadedFromDB = false;
            it = send_push_erase(it);
            break;
          }
          if (!eq) broadcast_push_back(pong);
        }
      };
      bool matchPong(const string &match, mOrderFilled *const pong) {
        for (auto it = begin(); it != end(); ++it) {
          if (it->tradeId != match) continue;
          Amount Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
          it->Ktime = pong->time;
          it->Kprice = ((Kqty*pong->price) + (it->Kqty*it->Kprice)) / (it->Kqty+Kqty);
          it->Kqty = it->Kqty + Kqty;
          it->Kvalue = abs(it->Kqty*it->Kprice);
          pong->quantity = pong->quantity - Kqty;
          pong->value = abs(pong->price*pong->quantity);
          if (it->quantity <= it->Kqty)
            it->Kdiff = ((it->quantity * it->price) - (it->Kqty * it->Kprice))
                      * (it->side == Side::Ask ? 1 : -1);
          it->isPong = true;
          it->loadedFromDB = false;
          it = send_push_erase(it);
          break;
        }
        return pong->quantity > 0;
      };
      void broadcast_push_back(const mOrderFilled &row) {
        rows.push_back(row);
        backup();
        if (crbegin()->Kqty < 0) rbegin()->Kqty = -2;
        broadcast();
      };
      iterator send_push_erase(iterator it) {
        mOrderFilled row = *it;
        it = rows.erase(it);
        broadcast_push_back(row);
        erase();
        return it;
      };
      string explainOK() const override {
        return "loaded % historical Trades";
      };
  };

  struct mRecentTrade {
     Price price    = 0;
    Amount quantity = 0;
     Clock time     = 0;
    mRecentTrade(const Price &p, const Amount &q)
      : price(p)
      , quantity(q)
      , time(Tstamp)
    {};
  };
  struct mRecentTrades {
    private_ref:
      const mQuotingParams &qp;
    public:
      mRecentTrades(const mQuotingParams &q)
        : qp(q)
      {};
    multimap<Price, mRecentTrade> buys,
                                  sells;
                           Amount sumBuys       = 0,
                                  sumSells      = 0;
                            Price lastBuyPrice  = 0,
                                  lastSellPrice = 0;
    void insert(const mLastOrder &order) {
      (order.side == Side::Bid
        ? lastBuyPrice
        : lastSellPrice
      ) = order.price;
      (order.side == Side::Bid
        ? buys
        : sells
      ).insert(pair<Price, mRecentTrade>(
        order.price,
        mRecentTrade(order.price, order.tradeQuantity)
      ));
    };
    void expire() {
      if (buys.size()) expire(&buys);
      if (sells.size()) expire(&sells);
      skip();
      sumBuys = sum(&buys);
      sumSells = sum(&sells);
    };
    private:
      const Amount sum(multimap<Price, mRecentTrade> *const k) const {
        Amount sum = 0;
        for (const auto &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void expire(multimap<Price, mRecentTrade> *const k) {
        Clock now = Tstamp;
        for (auto it = k->begin(); it != k->end();)
          if (it->second.time + qp.tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (buys.size() and sells.size()) {
          mRecentTrade &buy = buys.rbegin()->second;
          mRecentTrade &sell = sells.begin()->second;
          if (sell.price < buy.price) break;
          const Amount buyQty = buy.quantity;
          buy.quantity -= sell.quantity;
          sell.quantity -= buyQty;
          if (buy.quantity <= 0)
            buys.erase(buys.rbegin()->first);
          if (sell.quantity <= 0)
            sells.erase(sells.begin()->first);
        }
      };
  };

  struct mSafety: public Client::Broadcast<mSafety> {
              double buy      = 0,
                     sell     = 0,
                     combined = 0;
               Price buyPing  = 0,
                     sellPing = 0;
              Amount buySize  = 0,
                     sellSize = 0;
      mTradesHistory trades;
       mRecentTrades recentTrades;
    private_ref:
      const mQuotingParams &qp;
      const Price          &fairValue;
      const Amount         &baseValue,
                           &baseTotal,
                           &targetBasePosition;
    public:
      mSafety(const KryptoNinja &bot, const mQuotingParams &q, const mButtons &b, const Price &f, const Amount &v, const Amount &t, const Amount &p)
        : Broadcast(bot)
        , trades(bot, q, b)
        , recentTrades(q)
        , qp(q)
        , fairValue(f)
        , baseValue(v)
        , baseTotal(t)
        , targetBasePosition(p)
      {};
      void timer_1s() {
        calc();
      };
      void insertTrade(const mLastOrder &order) {
        recentTrades.insert(order);
        trades.insert(order);
        calc();
      };
      void calc() {
        if (!baseValue or !fairValue) return;
        calcSizes();
        calcPrices();
        recentTrades.expire();
        if (empty()) return;
        buy  = recentTrades.sumBuys / buySize;
        sell = recentTrades.sumSells / sellSize;
        combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
        broadcast();
      };
      const bool empty() const {
        return !baseValue or !buySize or !sellSize;
      };
      const mMatter about() const override {
        return mMatter::TradeSafetyValue;
      };
      const bool read_same_blob() const override {
        return false;
      };
    private:
      void calcSizes() {
        sellSize = qp.percentageValues
            ? qp.sellSizePercentage * baseValue / 1e+2
            : qp.sellSize;
        buySize = qp.percentageValues
          ? qp.buySizePercentage * baseValue / 1e+2
          : qp.buySize;
        if (qp.aggressivePositionRebalancing == mAPR::Off) return;
        if (qp.buySizeMax)
          buySize = fmax(buySize, targetBasePosition - baseTotal);
        if (qp.sellSizeMax)
          sellSize = fmax(sellSize, baseTotal - targetBasePosition);
      };
      void calcPrices() {
        if (qp.safety == mQuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
          if (qp.safety == mQuotingSafety::Off) return;
          Price widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * fairValue / 100
            : qp.widthPong;
          if (qp.safety == mQuotingSafety::PingPoing) {
            if (recentTrades.lastBuyPrice and fairValue > recentTrades.lastBuyPrice - widthPong)
              buyPing = recentTrades.lastBuyPrice;
            if (recentTrades.lastSellPrice and fairValue < recentTrades.lastSellPrice + widthPong)
              sellPing = recentTrades.lastSellPrice;
          } else {
            map<Price, mOrderFilled> tradesBuy;
            map<Price, mOrderFilled> tradesSell;
            for (const mOrderFilled &it: trades)
              (it.side == Side::Bid ? tradesBuy : tradesSell)[it.price] = it;
            Amount buyQty = 0,
                   sellQty = 0;
            if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
              matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
              matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
              if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
              if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
            } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
              matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
              matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
            } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
              matchAllPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
              matchAllPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
            }
            if (buyQty) buyPing /= buyQty;
            if (sellQty) sellPing /= sellQty;
          }
        }
      };
      void matchFirstPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, false, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(false, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width) {
        matchPing(false, false, tradesSide, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (auto it = tradesSide->crbegin(); it != tradesSide->crend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (const auto &it : *tradesSide)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it.second.price, it.second.quantity, it.second.price, it.second.Kqty, reverse))
            break;
      };
      const bool matchPing(bool _near, bool _far, Price *ping, Amount *qty, Amount qtyMax, Price width, Price fv, Price price, Amount qtyTrade, Price priceTrade, Amount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and KqtyTrade < qtyTrade
        ) {
          Amount qty_ = qtyTrade;
          if (_near or _far)
            qty_ = fmin(qtyMax - *qty, qty_);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax and (_near or _far);
      };
  };
  static void to_json(json &j, const mSafety &k) {
    j = {
      {     "buy", k.buy     },
      {    "sell", k.sell    },
      {"combined", k.combined},
      { "buyPing", k.buyPing },
      {"sellPing", k.sellPing}
    };
  };

  struct mTarget: public Sqlite::StructBackup<mTarget>,
                  public Client::Broadcast<mTarget> {
    Amount targetBasePosition = 0,
           positionDivergence = 0;
    private_ref:
      const KryptoNinja    &K;
      const mQuotingParams &qp;
      const double         &targetPositionAutoPercentage;
      const Amount         &baseValue;
    public:
      mTarget(const KryptoNinja &bot, const mQuotingParams &q, const double &t, const Amount &v)
        : StructBackup(bot)
        , Broadcast(bot)
        , K(bot)
        , qp(q)
        , targetPositionAutoPercentage(t)
        , baseValue(v)
      {};
      void calcTargetBasePos() {
        if (warn_empty()) return;
        targetBasePosition = K.gateway->decimal.amount.round(
          qp.autoPositionMode == mAutoPositionMode::Manual
            ? (qp.percentageValues
              ? qp.targetBasePositionPercentage * baseValue / 1e+2
              : qp.targetBasePosition)
            : targetPositionAutoPercentage * baseValue / 1e+2
        );
        calcPDiv();
        if (broadcast()) {
          backup();
          if (K.arg<int>("debug-wallet")) report();
        }
      };
      const bool warn_empty() const {
        const bool err = empty();
        if (err and (float)clock()/CLOCKS_PER_SEC > 3.0)
          Print::logWar("PG", "Unable to calculate TBP, missing wallet data");
        return err;
      };
      const bool empty() const {
        return !baseValue;
      };
      const bool realtime() const override {
        return false;
      };
      const mMatter about() const override {
        return mMatter::TargetBasePosition;
      };
      const bool read_same_blob() const override {
        return false;
      };
    private:
      void calcPDiv() {
        Amount pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * baseValue / 1e+2
          : qp.positionDivergence;
        if (qp.autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp.positionDivergenceMode)
          positionDivergence = pDiv;
        else {
          Amount pDivMin = qp.percentageValues
            ? qp.positionDivergencePercentageMin * baseValue / 1e+2
            : qp.positionDivergenceMin;
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          if (mPDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (mPDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (mPDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (mPDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
        positionDivergence = K.gateway->decimal.amount.round(positionDivergence);
      };
      void report() const {
        Print::log("PG", "TBP: "
          + to_string((int)(targetBasePosition / baseValue * 1e+2)) + "% = " + K.gateway->decimal.amount.str(targetBasePosition)
          + " " + K.gateway->base + ", pDiv: "
          + to_string((int)(positionDivergence / baseValue * 1e+2)) + "% = " + K.gateway->decimal.amount.str(positionDivergence)
          + " " + K.gateway->base);
      };
      const string explain() const override {
        return to_string(targetBasePosition);
      };
      string explainOK() const override {
        return "loaded TBP = % " + K.gateway->base;
      };
  };
  static void to_json(json &j, const mTarget &k) {
    j = {
      { "tbp", k.targetBasePosition},
      {"pDiv", k.positionDivergence}
    };
  };
  static void from_json(const json &j, mTarget &k) {
    k.targetBasePosition = j.value("tbp",  0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
  };

  struct mWalletPosition: public mWallets,
                          public Client::Broadcast<mWalletPosition> {
     mTarget target;
     mSafety safety;
    mProfits profits;
    private_ref:
      const KryptoNinja &K;
      const mOrders     &orders;
      const Price       &fairValue;
    public:
      mWalletPosition(const KryptoNinja &bot, const mQuotingParams &q, const mOrders &o, const mButtons &b, const mMarketLevels &l)
        : Broadcast(bot)
        , target(bot, q, l.stats.ewma.targetPositionAutoPercentage, base.value)
        , safety(bot, q, b, l.fairValue, base.value, base.total, target.targetBasePosition)
        , profits(bot, q)
        , K(bot)
        , orders(o)
        , fairValue(l.fairValue)
      {};
      const bool ready() const {
        return !safety.empty();
      };
      void read_from_gw(const mWallets &raw) {
        if (raw.base.currency.empty() or raw.quote.currency.empty() or !fairValue) return;
        base.currency = raw.base.currency;
        quote.currency = raw.quote.currency;
        calcMaxFunds(raw, K.arg<double>("wallet-limit"));
        calcFunds();
      };
      void calcFunds() {
        calcFundsSilently();
        broadcast();
      };
      void calcFundsAfterOrder(const mLastOrder &order, bool *const askForFees) {
        if (!order.price) return;
        calcHeldAmount(order.side);
        calcFundsSilently();
        if (order.tradeQuantity) {
          safety.insertTrade(order);
          *askForFees = true;
        }
      };
      const mMatter about() const override {
        return mMatter::Position;
      };
      const bool realtime() const override {
        return false;
      };
      const bool read_asap() const override {
        return false;
      };
      const bool read_same_blob() const override {
        return false;
      };
    private:
      void calcFundsSilently() {
        if (base.currency.empty() or quote.currency.empty() or !fairValue) return;
        calcValues();
        calcProfits();
        target.calcTargetBasePos();
      };
      void calcHeldAmount(const Side &side) {
        const Amount heldSide = orders.heldAmount(side);
        if (side == Side::Ask and !base.currency.empty())
          mWallet::reset(base.total - heldSide, heldSide, &base);
        else if (side == Side::Bid and !quote.currency.empty())
          mWallet::reset(quote.total - heldSide, heldSide, &quote);
      };
      void calcValues() {
        base.value  = (quote.total / fairValue) + base.total;
        quote.value = (base.total * fairValue) + quote.total;
      };
      void calcProfits() {
        if (!profits.ratelimit())
          profits.push_back({base.value, quote.value, Tstamp});
        base.profit  = profits.calcBaseDiff();
        quote.profit = profits.calcQuoteDiff();
      };
      void calcMaxFunds(mWallets raw, Amount limit) {
        if (limit) {
          limit -= raw.quote.held / fairValue;
          if (limit > 0 and raw.quote.amount / fairValue > limit) {
            raw.quote.amount = limit * fairValue;
            raw.base.amount = limit = 0;
          } else limit -= raw.quote.amount / fairValue;
          limit -= raw.base.held;
          if (limit > 0 and raw.base.amount > limit)
            raw.base.amount = limit;
        }
        mWallet::reset(raw.base.amount, raw.base.held, &base);
        mWallet::reset(raw.quote.amount, raw.quote.held, &quote);
      };
  };

  struct mQuote: public mLevel {
    const Side        side   = (Side)0;
          mQuoteState state  = mQuoteState::MissingData;
          bool        isPong = false;
    mQuote(const Side &s)
      : side(s)
    {};
    const bool empty() const {
      return !size or !price;
    };
    void skip() {
      size = 0;
    };
    void clear(const mQuoteState &reason) {
      price = size = 0;
      state = reason;
    };
    virtual const bool deprecates(const Price&) const = 0;
    const bool checkCrossed(const mQuote &opposite) {
      if (empty()) return false;
      if (opposite.empty() or deprecates(opposite.price)) {
        state = mQuoteState::Live;
        return false;
      }
      state = mQuoteState::Crossed;
      return true;
    };
  };
  struct mQuoteBid: public mQuote {
    mQuoteBid()
      : mQuote(Side::Bid)
    {};
    const bool deprecates(const Price &higher) const override {
      return price < higher;
    };
  };
  struct mQuoteAsk: public mQuote {
    mQuoteAsk()
      : mQuote(Side::Ask)
    {};
    const bool deprecates(const Price &lower) const override {
      return price > lower;
    };
  };
  struct mQuotes {
    mQuoteBid bid;
    mQuoteAsk ask;
         bool superSpread = false;
    private_ref:
      const KryptoNinja &K;
    public:
      mQuotes(const KryptoNinja &bot)
        : K(bot)
      {};
      void checkCrossedQuotes() {
        if ((unsigned int)bid.checkCrossed(ask)
          | (unsigned int)ask.checkCrossed(bid)
        ) Print::logWar("QE", "Crossed bid/ask quotes detected, that is.. unexpected");
      };
      void debug(const string &step) {
        if (K.arg<int>("debug-quotes"))
          Print::log("DEBUG QE", "[" + step + "] "
            + to_string((int)bid.state) + ":"
            + to_string((int)ask.state) + " "
            + ((json){{"bid", bid}, {"ask", ask}}).dump()
          );
      };
  };

  struct mDummyMarketMaker: public Client::Clicked::Catch {
    private:
      void (*calcRawQuotesFromMarket)(
        const mMarketLevels&,
        const Price&,
        const Price&,
        const Amount&,
        const Amount&,
              mQuotes&
      ) = nullptr;
    private_ref:
      const KryptoNinja     &K;
      const mQuotingParams  &qp;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
            mQuotes         &quotes;
    public:
      mDummyMarketMaker(const KryptoNinja &bot, const mQuotingParams &q, const mMarketLevels &l, const mWalletPosition &w, mQuotes &Q)
        : Catch(bot, {
            {&q, [&]() { mode(); }}
          })
        , K(bot)
        , qp(q)
        , levels(l)
        , wallet(w)
        , quotes(Q)
      {};
      void calcRawQuotes() const  {
        calcRawQuotesFromMarket(
          levels,
          K.gateway->minTick,
          levels.calcQuotesWidth(&quotes.superSpread),
          wallet.safety.buySize,
          wallet.safety.sellSize,
          quotes
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(mQuoteState::WidthMustBeSmaller);
          quotes.ask.clear(mQuoteState::WidthMustBeSmaller);
          Print::logWar("QP", "Negative price detected, widthPing must be smaller");
        }
      };
    private:
      void mode() {
        if (qp.mode == mQuotingMode::Top)              calcRawQuotesFromMarket = calcTopOfMarket;
        else if (qp.mode == mQuotingMode::Mid)         calcRawQuotesFromMarket = calcMidOfMarket;
        else if (qp.mode == mQuotingMode::Join)        calcRawQuotesFromMarket = calcJoinMarket;
        else if (qp.mode == mQuotingMode::InverseJoin) calcRawQuotesFromMarket = calcInverseJoinMarket;
        else if (qp.mode == mQuotingMode::InverseTop)  calcRawQuotesFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == mQuotingMode::HamelinRat)  calcRawQuotesFromMarket = calcColossusOfMarket;
        else if (qp.mode == mQuotingMode::Depth)       calcRawQuotesFromMarket = calcDepthOfMarket;
        else error("QE", "Invalid quoting mode saved, consider to remove the database file");
      };
      static void quoteAtTopOfMarket(const mMarketLevels &levels, const Price &minTick, mQuotes &quotes) {
        const mLevel &topBid = levels.bids.begin()->size > minTick
          ? levels.bids.at(0)
          : levels.bids.at(levels.bids.size() > 1 ? 1 : 0);
        const mLevel &topAsk = levels.asks.begin()->size > minTick
          ? levels.asks.at(0)
          : levels.asks.at(levels.asks.size() > 1 ? 1 : 0);
        quotes.bid.price = topBid.price;
        quotes.ask.price = topAsk.price;
      };
      static void calcTopOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price + minTick);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price - minTick);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcMidOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quotes.bid.price = fmax(levels.fairValue - widthPing, 0);
        quotes.ask.price = levels.fairValue + widthPing;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcJoinMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseJoinMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        Price mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseTopOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        Price mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        quotes.bid.price = quotes.bid.price + minTick;
        quotes.ask.price = quotes.ask.price - minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcColossusOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.size = 0;
        quotes.ask.size = 0;
        for (const mLevel &it : levels.bids)
          if (quotes.bid.size < it.size and it.price <= quotes.bid.price) {
            quotes.bid.size = it.size;
            quotes.bid.price = it.price;
          }
        for (const mLevel &it : levels.asks)
          if (quotes.ask.size < it.size and it.price >= quotes.ask.price) {
            quotes.ask.size = it.size;
            quotes.ask.price = it.price;
          }
        if (quotes.bid.size) quotes.bid.price += minTick;
        if (quotes.ask.size) quotes.ask.price -= minTick;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcDepthOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &depth,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        Price bidPx = levels.bids.cbegin()->price;
        Amount bidDepth = 0;
        for (const mLevel &it : levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        Price askPx = levels.asks.cbegin()->price;
        Amount askDepth = 0;
        for (const mLevel &it : levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        quotes.bid.price = bidPx;
        quotes.ask.price = askPx;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
  };

  struct mAntonioCalculon: public Client::Broadcast<mAntonioCalculon> {
                  mQuotes quotes;
        mDummyMarketMaker dummyMM;
    vector<const mOrder*> zombies;
             unsigned int countWaiting = 0,
                          countWorking = 0,
                          AK47inc      = 0;
                 mSideAPR sideAPR      = mSideAPR::Off;
    private_ref:
      const KryptoNinja     &K;
      const mQuotingParams  &qp;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
    public:
      mAntonioCalculon(const KryptoNinja &bot, const mQuotingParams &q, const mMarketLevels &l, const mWalletPosition &w)
        : Broadcast(bot)
        , quotes(bot)
        , dummyMM(bot, q, l, w, quotes)
        , K(bot)
        , qp(q)
        , levels(l)
        , wallet(w)
      {};
      vector<const mOrder*> clear() {
        broadcast();
        states(mQuoteState::MissingData);
        countWaiting =
        countWorking = 0;
        vector<const mOrder*> zombies_;
        zombies.swap(zombies_);
        return zombies_;
      };
      void offline() {
        states(mQuoteState::Disconnected);
      };
      void paused() {
        states(mQuoteState::DisabledQuotes);
      };
      void calcQuotes() {
        states(mQuoteState::UnknownHeld);
        dummyMM.calcRawQuotes();
        applyQuotingParameters();
      };
      const bool abandon(const mOrder &order, mQuote &quote, unsigned int &bullets) {
        if (stillAlive(order)) {
          if (abs(order.price - quote.price) < K.gateway->minTick)
            quote.skip();
          else if (order.status == Status::Waiting) {
            if (qp.safety != mQuotingSafety::AK47
              or !--bullets
            ) quote.skip();
          } else if (qp.safety != mQuotingSafety::AK47
            or quote.deprecates(order.price)
          ) {
            if (K.arg<int>("lifetime") and order.time + K.arg<int>("lifetime") > Tstamp)
              quote.skip();
            else return true;
          }
        }
        return false;
      };
      const mMatter about() const override {
        return mMatter::QuoteStatus;
      };
      const bool realtime() const override {
        return false;
      };
      const bool read_same_blob() const override {
        return false;
      };
    private:
      void states(const mQuoteState &state) {
        quotes.bid.state =
        quotes.ask.state = state;
      };
      const bool stillAlive(const mOrder &order) {
        if (order.status == Status::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
          ++countWaiting;
        } else ++countWorking;
        return !order.disablePostOnly;
      };
      void applyQuotingParameters() {
        quotes.debug("?"); applySuperTrades();
        quotes.debug("A"); applyEwmaProtection();
        quotes.debug("B"); applyTotalBasePosition();
        quotes.debug("C"); applyStdevProtection();
        quotes.debug("D"); applyAggressivePositionRebalancing();
        quotes.debug("E"); applyAK47Increment();
        quotes.debug("F"); applyBestWidth();
        quotes.debug("G"); applyTradesPerMinute();
        quotes.debug("H"); applyRoundPrice();
        quotes.debug("I"); applyRoundSize();
        quotes.debug("J"); applyDepleted();
        quotes.debug("K"); applyWaitingPing();
        quotes.debug("L"); applyEwmaTrendProtection();
        quotes.debug("!");
        quotes.checkCrossedQuotes();
      };
      void applySuperTrades() {
        if (!quotes.superSpread
          or (qp.superTrades != mSOP::Size and qp.superTrades != mSOP::TradesSize)
        ) return;
        if (!qp.buySizeMax and !quotes.bid.empty())
          quotes.bid.size = fmin(
            qp.sopSizeMultiplier * quotes.bid.size,
            (wallet.quote.amount / quotes.bid.price) / 2
          );
        if (!qp.sellSizeMax and !quotes.ask.empty())
          quotes.ask.size = fmin(
            qp.sopSizeMultiplier * quotes.ask.size,
            wallet.base.amount / 2
          );
      };
      void applyEwmaProtection() {
        if (!qp.protectionEwmaQuotePrice or !levels.stats.ewma.mgEwmaP) return;
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(levels.stats.ewma.mgEwmaP, quotes.ask.price);
        if (!quotes.bid.empty())
          quotes.bid.price = fmin(levels.stats.ewma.mgEwmaP, quotes.bid.price);
      };
      void applyTotalBasePosition() {
        if (wallet.base.total < wallet.target.targetBasePosition - wallet.target.positionDivergence) {
          quotes.ask.clear(mQuoteState::TBPHeld);
          if (!quotes.bid.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = mSideAPR::Buy;
            if (!qp.buySizeMax)
              quotes.bid.size = fmin(
                qp.aprMultiplier * quotes.bid.size,
                wallet.target.targetBasePosition - wallet.base.total
              );
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          quotes.bid.clear(mQuoteState::TBPHeld);
          if (!quotes.ask.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = mSideAPR::Sell;
            if (!qp.sellSizeMax)
              quotes.ask.size = fmin(
                qp.aprMultiplier * quotes.ask.size,
                wallet.base.total - wallet.target.targetBasePosition
              );
          }
        }
        else sideAPR = mSideAPR::Off;
      };
      void applyStdevProtection() {
        if (qp.quotingStdevProtection == mSTDEV::Off or !levels.stats.stdev.fair) return;
        if (!quotes.ask.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != mSideAPR::Sell
        ))
          quotes.ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean
                : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean
                  : levels.stats.stdev.askMean)
              : levels.fairValue
            ) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
              ? levels.stats.stdev.fair
              : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                ? levels.stats.stdev.top
                : levels.stats.stdev.ask)),
            quotes.ask.price
          );
        if (!quotes.bid.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != mSideAPR::Buy
        ))
          quotes.bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean
                : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean
                  : levels.stats.stdev.bidMean)
              : levels.fairValue
            ) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
              ? levels.stats.stdev.fair
              : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                ? levels.stats.stdev.top
                : levels.stats.stdev.bid)),
            quotes.bid.price
          );
      };
      void applyAggressivePositionRebalancing() {
        if (qp.safety == mQuotingSafety::Off) return;
        const Price widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        if (!quotes.ask.empty() and wallet.safety.buyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == mSideAPR::Sell)
            or ((qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
              ? quotes.ask.price < wallet.safety.buyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.ask.price = max(levels.bids.at(0).price + K.gateway->minTick, wallet.safety.buyPing + widthPong);
          quotes.ask.isPong = quotes.ask.price >= wallet.safety.buyPing + widthPong;
        }
        if (!quotes.bid.empty() and wallet.safety.sellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == mSideAPR::Buy)
            or ((qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
              ? quotes.bid.price > wallet.safety.sellPing - widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.bid.price = min(levels.asks.at(0).price - K.gateway->minTick, wallet.safety.sellPing - widthPong);
          quotes.bid.isPong = quotes.bid.price <= wallet.safety.sellPing - widthPong;
        }
      };
      void applyAK47Increment() {
        if (qp.safety != mQuotingSafety::AK47) return;
        const Price range = qp.percentageValues
          ? qp.rangePercentage * levels.fairValue / 100
          : qp.range;
        if (!quotes.bid.empty())
          quotes.bid.price -= AK47inc * range;
        if (!quotes.ask.empty())
          quotes.ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyBestWidth() {
        if (!qp.bestWidth) return;
        const Amount bestWidthSize = (sideAPR == mSideAPR::Off ? qp.bestWidthSize : 0);
        Amount depth = 0;
        if (!quotes.ask.empty())
          for (const mLevel &it : levels.asks)
            if (it.price > quotes.ask.price) {
              depth += it.size;
              if (depth <= bestWidthSize) continue;
              const Price bestAsk = it.price - K.gateway->minTick;
              if (bestAsk > quotes.ask.price)
                quotes.ask.price = bestAsk;
              break;
            }
        depth = 0;
        if (!quotes.bid.empty())
          for (const mLevel &it : levels.bids)
            if (it.price < quotes.bid.price) {
              depth += it.size;
              if (depth <= bestWidthSize) continue;
              const Price bestBid = it.price + K.gateway->minTick;
              if (bestBid < quotes.bid.price)
                quotes.bid.price = bestBid;
              break;
            }
      };
      void applyTradesPerMinute() {
        const double factor = (quotes.superSpread and (
          qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize
        )) ? qp.sopWidthMultiplier
           : 1;
        if (wallet.safety.sell >= qp.tradesPerMinute * factor)
          quotes.ask.clear(mQuoteState::MaxTradesSeconds);
        if (wallet.safety.buy >= qp.tradesPerMinute * factor)
          quotes.bid.clear(mQuoteState::MaxTradesSeconds);
      };
      void applyRoundPrice() {
        if (!quotes.bid.empty())
          quotes.bid.price = fmax(
            0,
            round(quotes.bid.price / K.gateway->minTick) * K.gateway->minTick
          );
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(
            quotes.bid.price + K.gateway->minTick,
            round(quotes.ask.price / K.gateway->minTick) * K.gateway->minTick
          );
      };
      void applyRoundSize() {
        if (!quotes.bid.empty())
          quotes.bid.size = K.gateway->decimal.amount.round(
            fmax(K.gateway->minSize, fmin(
              quotes.bid.size,
              K.gateway->decimal.amount.floor(
                wallet.quote.total / (quotes.bid.price * (1.0 + K.gateway->makeFee))
              )
            ))
          );
        if (!quotes.ask.empty())
          quotes.ask.size = K.gateway->decimal.amount.round(
            fmax(K.gateway->minSize, fmin(
              quotes.ask.size,
              K.gateway->decimal.amount.floor(
                wallet.base.total
              )
            ))
          );
      };
      void applyDepleted() {
        if (!quotes.bid.empty()
          and wallet.quote.total / quotes.bid.price < K.gateway->minSize * (1.0 + K.gateway->makeFee)
        ) quotes.bid.clear(mQuoteState::DepletedFunds);
        if (!quotes.ask.empty()
          and wallet.base.total < K.gateway->minSize * (1.0 + K.gateway->makeFee)
        ) quotes.ask.clear(mQuoteState::DepletedFunds);
      };
      void applyWaitingPing() {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!quotes.ask.isPong and (
          (quotes.bid.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) quotes.ask.clear(mQuoteState::WaitingPing);
        if (!quotes.bid.isPong and (
          (quotes.ask.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) quotes.bid.clear(mQuoteState::WaitingPing);
      };
      void applyEwmaTrendProtection() {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold)
          quotes.ask.clear(mQuoteState::UpTrendHeld);
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold)
          quotes.bid.clear(mQuoteState::DownTrendHeld);
      };
  };
  static void to_json(json &j, const mAntonioCalculon &k) {
    j = {
      {            "bidStatus", k.quotes.bid.state},
      {            "askStatus", k.quotes.ask.state},
      {              "sideAPR", k.sideAPR         },
      {"quotesInMemoryWaiting", k.countWaiting    },
      {"quotesInMemoryWorking", k.countWorking    },
      {"quotesInMemoryZombies", k.zombies.size()  }
    };
  };

  struct mSemaphore: public Client::Broadcast<mSemaphore>,
                     public Client::Clickable,
                     public Hotkey::Catch {
    Connectivity greenButton  = Connectivity::Disconnected,
                 greenGateway = Connectivity::Disconnected;
    private:
      Connectivity adminAgreement = Connectivity::Disconnected;
    private_ref:
      const KryptoNinja &K;
    public:
      mSemaphore(const KryptoNinja &bot)
        : Broadcast(bot)
        , Clickable(bot)
        , Catch(bot, {
            {'Q', [&]() { exit(); }},
            {'q', [&]() { exit(); }},
            {'\e', [&]() { toggle(); }}
          })
        , K(bot)
      {};
      void click(const json &j) override {
        if (j.is_object()
          and j.at("agree").is_number()
          and j.at("agree").get<Connectivity>() != adminAgreement
        ) toggle();
      };
      const bool paused() const {
        return !(bool)greenButton;
      };
      const bool offline() const {
        return !(bool)greenGateway;
      };
      void agree(const bool &agreement) {
        adminAgreement = (Connectivity)agreement;
      };
      void read_from_gw(const Connectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          switchFlag();
        }
      };
      const mMatter about() const override {
        return mMatter::Connectivity;
      };
    private:
      void toggle() {
        agree(!(bool)adminAgreement);
        switchFlag();
      };
      void switchFlag() {
        const Connectivity previous = greenButton;
        greenButton = (Connectivity)(
          (bool)greenGateway and (bool)adminAgreement
        );
        if (greenButton != previous)
          Print::log("GW " + K.gateway->exchange, "Quoting state changed to",
            string(paused() ? "DIS" : "") + "CONNECTED");
        broadcast();
        Print::repaint();
      };
  };
  static void to_json(json &j, const mSemaphore &k) {
    j = {
      { "agree", k.greenButton },
      {"online", k.greenGateway}
    };
  };

  struct mBroker: public Client::Clicked::Catch {
          mSemaphore semaphore;
    mAntonioCalculon calculon;
    private_ref:
      const KryptoNinja    &K;
      const mQuotingParams &qp;
            mOrders        &orders;
    public:
      mBroker(const KryptoNinja &bot, const mQuotingParams &q, mOrders &o, const mButtons &b, const mMarketLevels &l, const mWalletPosition &w)
        : Catch(bot, {
            {&b.submit, [&](const json &j) { placeOrder(j); }},
            {&b.cancel, [&](const json &j) { cancelOrder(orders.find(j)); }},
            {&b.cancelAll, [&]() { cancelOrders(); }}
          })
        , semaphore(bot)
        , calculon(bot, q, l, w)
        , K(bot)
        , qp(q)
        , orders(o)
      {};
      const bool ready() {
        if (semaphore.offline()) {
          calculon.offline();
          return false;
        }
        return true;
      };
      const bool calcQuotes() {
        if (semaphore.paused()) {
          calculon.paused();
          return false;
        }
        calculon.calcQuotes();
        return true;
      };
      const vector<mOrder*> abandon(mQuote &quote) {
        vector<mOrder*> abandoned;
        unsigned int bullets = qp.bullets;
        const bool all = quote.state != mQuoteState::Live;
        for (mOrder *const it : orders.at(quote.side))
          if (all or calculon.abandon(*it, quote, bullets))
            abandoned.push_back(it);
        return abandoned;
      };
      void clear() {
        for (const mOrder *const it : calculon.clear())
          orders.purge(it);
      };
      void placeOrder(const mOrder &raw) {
        K.gateway->place(orders.upsert(raw));
      };
      void replaceOrder(const Price &price, const bool &isPong, mOrder *const order) {
        if (orders.replace(price, isPong, order))
          K.gateway->replace(order);
      };
      void cancelOrder(mOrder *const order) {
        if (orders.cancel(order))
          K.gateway->cancel(order);
      };
      void cancelOrders() {
        for (mOrder *const it : orders.working())
          cancelOrder(it);
      };
  };

  class mProduct: public Client::Broadcast<mProduct> {
    private_ref:
      const KryptoNinja &K;
    public:
      mProduct(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      const json to_json() const {
        return {
          {   "exchange", K.gateway->exchange        },
          {       "base", K.gateway->base            },
          {      "quote", K.gateway->quote           },
          {    "minTick", K.gateway->minTick         },
          {       "inet", K.arg<string>("interface") },
          {"environment", K.arg<string>("title")     },
          { "matryoshka", K.arg<string>("matryoshka")}
        };
      };
      const mMatter about() const override {
        return mMatter::ProductAdvertisement;
      };
  };
  static void to_json(json &j, const mProduct &k) {
    j = k.to_json();
  };

  class mMemory: public Client::Broadcast<mMemory> {
    public:
      unsigned int orders_60s = 0;
    private:
      mProduct product;
    private_ref:
      const KryptoNinja &K;
    public:
      mMemory(const KryptoNinja &bot)
        : Broadcast(bot)
        , product(bot)
        , K(bot)
      {};
      void timer_60s() {
        broadcast();
        orders_60s = 0;
      };
      const json to_json() const {
        return {
          {  "addr", K.gateway->unlock           },
          {  "freq", orders_60s                  },
          { "theme", K.arg<int>("ignore-moon")
                       + K.arg<int>("ignore-sun")},
          {"memory", K.memSize()                 },
          {"dbsize", K.dbSize()                  }
        };
      };
      const mMatter about() const override {
        return mMatter::ApplicationState;
      };
  };
  static void to_json(json &j, const mMemory &k) {
    j = k.to_json();
  };
}

#endif

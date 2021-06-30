//! \file
//! \brief Trading logistics (welcome user! forked from https://github.com/michaelgrosner/tribeca).

namespace tribeca {
  enum class QuotingMode: unsigned int {
    Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth
  };
  enum class OrderPctTotal: unsigned int {
    Value, Side, TBPValue, TBPSide
  };
  enum class QuotingSafety: unsigned int {
    Off, PingPong, PingPoing, Boomerang, AK47
  };
  enum class QuoteState: unsigned int {
    Disconnected,  Live,             DisabledQuotes,
    MissingData,   UnknownHeld,      WidthTooHigh,
    TBPHeld,       MaxTradesSeconds, WaitingPing,
    DepletedFunds, Crossed,
    UpTrendHeld,   DownTrendHeld
  };
  enum class FairValueModel: unsigned int {
    BBO, wBBO, rwBBO
  };
  enum class AutoPositionMode: unsigned int {
    Manual, EWMA_LS, EWMA_LMS, EWMA_4
  };
  enum class PDivMode: unsigned int {
    Manual, Linear, Sine, SQRT, Switch
  };
  enum class APR: unsigned int {
    Off, Size, SizeWidth
  };
  enum class SideAPR: unsigned int {
    Off, Buy, Sell
  };
  enum class SOP: unsigned int {
    Off, Trades, Size, TradesSize
  };
  enum class STDEV: unsigned int {
    Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff
  };
  enum class PingAt: unsigned int {
    BothSides,    BidSide,         AskSide,
    DepletedSide, DepletedBidSide, DepletedAskSide,
    StopPings
  };
  enum class PongAt: unsigned int {
    ShortPingFair,       AveragePingFair,       LongPingFair,
    ShortPingAggressive, AveragePingAggressive, LongPingAggressive
  };

  struct QuotingParams: public Sqlite::StructBackup<QuotingParams>,
                        public Client::Broadcast<QuotingParams>,
                        public Client::Clickable {
    Price            widthPing                       = 300.0;
    double           widthPingPercentage             = 0.25;
    Price            widthPong                       = 300.0;
    double           widthPongPercentage             = 0.25;
    bool             widthPercentage                 = false;
    bool             bestWidth                       = true;
    Amount           bestWidthSize                   = 0;
    OrderPctTotal    orderPctTotal                   = OrderPctTotal::Value;
    double           tradeSizeTBPExp                 = 2.0;
    Amount           buySize                         = 0.02;
    double           buySizePercentage               = 7.0;
    bool             buySizeMax                      = false;
    Amount           sellSize                        = 0.01;
    double           sellSizePercentage              = 7.0;
    bool             sellSizeMax                     = false;
    PingAt           pingAt                          = PingAt::BothSides;
    PongAt           pongAt                          = PongAt::ShortPingFair;
    QuotingMode      mode                            = QuotingMode::Top;
    QuotingSafety    safety                          = QuotingSafety::PingPong;
    unsigned int     bullets                         = 2;
    Price            range                           = 0.5;
    double           rangePercentage                 = 5.0;
    FairValueModel   fvModel                         = FairValueModel::BBO;
    Amount           targetBasePosition              = 1.0;
    double           targetBasePositionPercentage    = 50.0;
    Amount           targetBasePositionMin           = 0.1;
    double           targetBasePositionPercentageMin = 10.0;
    Amount           targetBasePositionMax           = 1;
    double           targetBasePositionPercentageMax = 90.0;
    Amount           positionDivergence              = 0.9;
    Amount           positionDivergenceMin           = 0.4;
    double           positionDivergencePercentage    = 21.0;
    double           positionDivergencePercentageMin = 10.0;
    PDivMode         positionDivergenceMode          = PDivMode::Manual;
    bool             percentageValues                = false;
    AutoPositionMode autoPositionMode                = AutoPositionMode::EWMA_LS;
    APR              aggressivePositionRebalancing   = APR::Off;
    SOP              superTrades                     = SOP::Off;
    unsigned int     tradesPerMinute                 = 1;
    unsigned int     tradeRateSeconds                = 3;
    bool             protectionEwmaWidthPing         = false;
    bool             protectionEwmaQuotePrice        = true;
    unsigned int     protectionEwmaPeriods           = 200;
    STDEV            quotingStdevProtection          = STDEV::Off;
    bool             quotingStdevBollingerBands      = false;
    double           quotingStdevProtectionFactor    = 1.0;
    unsigned int     quotingStdevProtectionPeriods   = 1200;
    double           ewmaSensiblityPercentage        = 0.5;
    bool             quotingEwmaTrendProtection      = false;
    double           quotingEwmaTrendThreshold       = 2.0;
    unsigned int     veryLongEwmaPeriods             = 400;
    unsigned int     longEwmaPeriods                 = 200;
    unsigned int     mediumEwmaPeriods               = 100;
    unsigned int     shortEwmaPeriods                = 50;
    unsigned int     extraShortEwmaPeriods           = 12;
    unsigned int     ultraShortEwmaPeriods           = 3;
    double           aprMultiplier                   = 2;
    double           sopWidthMultiplier              = 2;
    double           sopSizeMultiplier               = 2;
    double           sopTradesMultiplier             = 2;
    bool             cancelOrdersAuto                = false;
    unsigned int     lifetime                        = 0;
    double           cleanPongsAuto                  = 0.0;
    double           profitHourInterval              = 0.5;
    bool             audio                           = false;
    unsigned int     delayUI                         = 3;
    int              _diffEwma                       = -1;
    private_ref:
      const KryptoNinja &K;
    public:
      QuotingParams(const KryptoNinja &bot)
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
        widthPing                       = fmax(K.gateway->tickPrice,   j.value("widthPing", widthPing));
        widthPingPercentage             = fmin(1e+2, fmax(1e-3,        j.value("widthPingPercentage", widthPingPercentage)));
        widthPong                       = fmax(K.gateway->tickPrice,   j.value("widthPong", widthPong));
        widthPongPercentage             = fmin(1e+2, fmax(1e-3,        j.value("widthPongPercentage", widthPongPercentage)));
        widthPercentage                 =                              j.value("widthPercentage", widthPercentage);
        bestWidth                       =                              j.value("bestWidth", bestWidth);
        bestWidthSize                   = fmax(0,                      j.value("bestWidthSize", bestWidthSize));
        orderPctTotal                   =                              j.value("orderPctTotal", orderPctTotal);
        tradeSizeTBPExp                 =                              j.value("tradeSizeTBPExp", tradeSizeTBPExp);
        buySize                         = fmax(K.gateway->minSize,     j.value("buySize", buySize));
        buySizePercentage               = fmin(1e+2, fmax(1e-3,        j.value("buySizePercentage", buySizePercentage)));
        buySizeMax                      =                              j.value("buySizeMax", buySizeMax);
        sellSize                        = fmax(K.gateway->minSize,     j.value("sellSize", sellSize));
        sellSizePercentage              = fmin(1e+2, fmax(1e-3,        j.value("sellSizePercentage", sellSizePercentage)));
        sellSizeMax                     =                              j.value("sellSizeMax", sellSizeMax);
        pingAt                          =                              j.value("pingAt", pingAt);
        pongAt                          =                              j.value("pongAt", pongAt);
        mode                            =                              j.value("mode", mode);
        safety                          =                              j.value("safety", safety);
        bullets                         = fmin(10, fmax(1,             j.value("bullets", bullets)));
        range                           =                              j.value("range", range);
        rangePercentage                 = fmin(1e+2, fmax(1e-3,        j.value("rangePercentage", rangePercentage)));
        fvModel                         =                              j.value("fvModel", fvModel);
        targetBasePosition              =                              j.value("targetBasePosition", targetBasePosition);
        targetBasePositionPercentage    = fmin(1e+2, fmax(0,           j.value("targetBasePositionPercentage", targetBasePositionPercentage)));
        targetBasePositionMin           =                              j.value("targetBasePositionMin", targetBasePositionMin);
        targetBasePositionPercentageMin = fmin(1e+2, fmax(0,           j.value("targetBasePositionPercentageMin", targetBasePositionPercentageMin)));
        targetBasePositionMax           =                              j.value("targetBasePositionMax", targetBasePositionMax);
        targetBasePositionPercentageMax = fmin(1e+2, fmax(0,           j.value("targetBasePositionPercentageMax", targetBasePositionPercentageMax)));
        positionDivergenceMin           =                              j.value("positionDivergenceMin", positionDivergenceMin);
        positionDivergenceMode          =                              j.value("positionDivergenceMode", positionDivergenceMode);
        positionDivergence              =                              j.value("positionDivergence", positionDivergence);
        positionDivergencePercentage    = fmin(1e+2, fmax(0,           j.value("positionDivergencePercentage", positionDivergencePercentage)));
        positionDivergencePercentageMin = fmin(1e+2, fmax(0,           j.value("positionDivergencePercentageMin", positionDivergencePercentageMin)));
        percentageValues                =                              j.value("percentageValues", percentageValues);
        autoPositionMode                =                              j.value("autoPositionMode", autoPositionMode);
        aggressivePositionRebalancing   =                              j.value("aggressivePositionRebalancing", aggressivePositionRebalancing);
        superTrades                     =                              j.value("superTrades", superTrades);
        tradesPerMinute                 = fmax(0,                      j.value("tradesPerMinute", tradesPerMinute));
        tradeRateSeconds                = fmax(0,                      j.value("tradeRateSeconds", tradeRateSeconds));
        protectionEwmaWidthPing         =                              j.value("protectionEwmaWidthPing", protectionEwmaWidthPing);
        protectionEwmaQuotePrice        =                              j.value("protectionEwmaQuotePrice", protectionEwmaQuotePrice);
        protectionEwmaPeriods           = fmax(1,                      j.value("protectionEwmaPeriods", protectionEwmaPeriods));
        quotingStdevProtection          =                              j.value("quotingStdevProtection", quotingStdevProtection);
        quotingStdevBollingerBands      =                              j.value("quotingStdevBollingerBands", quotingStdevBollingerBands);
        quotingStdevProtectionFactor    =                              j.value("quotingStdevProtectionFactor", quotingStdevProtectionFactor);
        quotingStdevProtectionPeriods   = fmax(1,                      j.value("quotingStdevProtectionPeriods", quotingStdevProtectionPeriods));
        ewmaSensiblityPercentage        =                              j.value("ewmaSensiblityPercentage", ewmaSensiblityPercentage);
        quotingEwmaTrendProtection      =                              j.value("quotingEwmaTrendProtection", quotingEwmaTrendProtection);
        quotingEwmaTrendThreshold       =                              j.value("quotingEwmaTrendThreshold", quotingEwmaTrendThreshold);
        veryLongEwmaPeriods             = fmax(1,                      j.value("veryLongEwmaPeriods", veryLongEwmaPeriods));
        longEwmaPeriods                 = fmax(1,                      j.value("longEwmaPeriods", longEwmaPeriods));
        mediumEwmaPeriods               = fmax(1,                      j.value("mediumEwmaPeriods", mediumEwmaPeriods));
        shortEwmaPeriods                = fmax(1,                      j.value("shortEwmaPeriods", shortEwmaPeriods));
        extraShortEwmaPeriods           = fmax(1,                      j.value("extraShortEwmaPeriods", extraShortEwmaPeriods));
        ultraShortEwmaPeriods           = fmax(1,                      j.value("ultraShortEwmaPeriods", ultraShortEwmaPeriods));
        aprMultiplier                   =                              j.value("aprMultiplier", aprMultiplier);
        sopWidthMultiplier              =                              j.value("sopWidthMultiplier", sopWidthMultiplier);
        sopSizeMultiplier               =                              j.value("sopSizeMultiplier", sopSizeMultiplier);
        sopTradesMultiplier             =                              j.value("sopTradesMultiplier", sopTradesMultiplier);
        cancelOrdersAuto                =                              j.value("cancelOrdersAuto", cancelOrdersAuto);
        lifetime                        = fmax(K.arg<int>("lifetime"), j.value("lifetime", lifetime));
        cleanPongsAuto                  =                              j.value("cleanPongsAuto", cleanPongsAuto);
        profitHourInterval              =                              j.value("profitHourInterval", profitHourInterval);
        audio                           =                              j.value("audio", audio);
        delayUI                         = fmax(0,                      j.value("delayUI", delayUI));
        if (mode == QuotingMode::Depth)
          widthPercentage = false;
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
      mMatter about() const override {
        return mMatter::QuotingParameters;
      };
    private:
      string explain() const override {
        return "Quoting Parameters";
      };
      string explainKO() const override {
        return "using default values for %";
      };
  };
  static void to_json(json &j, const QuotingParams &k) {
    j = {
      {                      "widthPing", k.widthPing                      },
      {            "widthPingPercentage", k.widthPingPercentage            },
      {                      "widthPong", k.widthPong                      },
      {            "widthPongPercentage", k.widthPongPercentage            },
      {                "widthPercentage", k.widthPercentage                },
      {                      "bestWidth", k.bestWidth                      },
      {                  "bestWidthSize", k.bestWidthSize                  },
      {                  "orderPctTotal", k.orderPctTotal                  },
      {                "tradeSizeTBPExp", k.tradeSizeTBPExp                },
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
      {          "targetBasePositionMin", k.targetBasePositionMin          },
      {"targetBasePositionPercentageMin", k.targetBasePositionPercentageMin},
      {          "targetBasePositionMax", k.targetBasePositionMax          },
      {"targetBasePositionPercentageMax", k.targetBasePositionPercentageMax},
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
      {                       "lifetime", k.lifetime                       },
      {                 "cleanPongsAuto", k.cleanPongsAuto                 },
      {             "profitHourInterval", k.profitHourInterval             },
      {                          "audio", k.audio                          },
      {                        "delayUI", k.delayUI                        }
    };
  };
  static void from_json(const json &j, QuotingParams &k) {
    k.from_json(j);
  };

  struct LastOrder {
     Price price;
    Amount filled;
      Side side;
      bool isPong;
  };
  struct Orders: public Client::Broadcast<Orders> {
    LastOrder updated;
    private:
      unordered_map<string, Order> orders;
    private_ref:
      const KryptoNinja &K;
    public:
      Orders(const KryptoNinja &bot)
        : Broadcast(bot)
        , updated()
        , K(bot)
      {};
      Order *find(const string &orderId) {
        return (orderId.empty()
          or orders.find(orderId) == orders.end()
        ) ? nullptr
          : &orders.at(orderId);
      };
      Order *findsert(const Order &raw) {
        if (raw.status == Status::Waiting and !raw.orderId.empty())
          return &(orders[raw.orderId] = raw);
        if (raw.orderId.empty() and !raw.exchangeId.empty()) {
          auto it = find_if(
            orders.begin(), orders.end(),
            [&](const pair<string, Order> &it_) {
              return raw.exchangeId == it_.second.exchangeId;
            }
          );
          if (it != orders.end())
            return &it->second;
        }
        return find(raw.orderId);
      };
      Amount heldAmount(const Side &side) const {
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
      vector<Order*> at(const Side &side) {
        vector<Order*> sideOrders;
        for (auto &it : orders)
          if (side == it.second.side)
             sideOrders.push_back(&it.second);
        return sideOrders;
      };
      vector<Order*> open() {
        vector<Order*> autoOrders;
        for (auto &it : orders)
          if (!it.second.manual)
            autoOrders.push_back(&it.second);
        return autoOrders;
      };
      vector<Order*> working() {
        vector<Order*> workingOrders;
        for (auto &it : orders)
          if (Status::Working == it.second.status
            and !it.second.manual
          ) workingOrders.push_back(&it.second);
        return workingOrders;
      };
      vector<Order> working(const bool &sorted = false) const {
        vector<Order> workingOrders;
        for (const auto &it : orders)
          if (Status::Working == it.second.status)
            workingOrders.push_back(it.second);
        if (sorted)
          sort(workingOrders.begin(), workingOrders.end(),
            [](const Order &a, const Order &b) {
              return a.price > b.price;
            }
          );
        return workingOrders;
      };
      Order *upsert(const Order &raw) {
        Order *const order = findsert(raw);
        Order::update(raw, order);
        if (K.arg<int>("debug-orders")) {
          report(order, " saved ");
          report_size();
        }
        return order;
      };
      bool replace(const Price &price, const bool &isPong, Order *const order) {
        const bool allowed = Order::replace(price, isPong, order);
        if (allowed and K.arg<int>("debug-orders")) report(order, "replace");
        return allowed;
      };
      bool cancel(Order *const order) {
        const bool allowed = Order::cancel(order);
        if (allowed and K.arg<int>("debug-orders")) report(order, "cancel ");
        return allowed;
      };
      void purge(const Order *const order) {
        if (K.arg<int>("debug-orders")) report(order, " purge ");
        orders.erase(order->orderId);
        if (K.arg<int>("debug-orders")) report_size();
      };
      void read_from_gw(const Order &raw) {
        if (K.arg<int>("debug-orders")) report(&raw, " reply ");
        Order *const order = upsert(raw);
        if (!order) {
          updated = {};
          return;
        }
        updated = {
          order->price,
          raw.filled,
          order->side,
          order->isPong
        };
        if (order->status == Status::Terminated)
          purge(order);
        broadcast();
        K.repaint();
      };
      mMatter about() const override {
        return mMatter::OrderStatusReports;
      };
      bool realtime() const override {
        return false;
      };
      json blob() const override {
        return working();
      };
    private:
      void report(const Order *const order, const string &reason) const {
        K.log("DEBUG OG", " " + reason + " " + (
          order
            ? order->orderId + "::" + order->exchangeId
              + " [" + to_string((int)order->status) + "]: "
              + K.gateway->decimal.amount.str(order->quantity) + " " + K.gateway->base + " at price "
              + K.gateway->decimal.price.str(order->price) + " " + K.gateway->quote
            : "not found"
        ));
      };
      void report_size() const {
        K.log("DEBUG OG", "memory " + to_string(orders.size()));
      };
  };
  static void to_json(json &j, const Orders &k) {
    j = k.blob();
  };

  struct MarketTakers: public Client::Broadcast<Trade> {
    vector<Trade>  trades;
            Amount takersBuySize60s  = 0,
                   takersSellSize60s = 0;
    public:
      MarketTakers(const KryptoNinja &bot)
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
      void read_from_gw(const Trade &raw) {
        trades.push_back(raw);
        broadcast();
      };
      mMatter about() const override {
        return mMatter::MarketTrade;
      };
      json blob() const override {
        return trades.back();
      };
      json hello() override {
        return trades;
      };
  };

  struct FairLevelsPrice: public Client::Broadcast<FairLevelsPrice> {
    private_ref:
      const Price &fairValue;
    public:
      FairLevelsPrice(const KryptoNinja &bot, const Price &f)
        : Broadcast(bot)
        , fairValue(f)
      {};
      json to_json() const {
        return {
          {"price", fairValue}
        };
      };
      mMatter about() const override {
        return mMatter::FairValue;
      };
      bool realtime() const override {
        return false;
      };
      bool read_same_blob() const override {
        return false;
      };
      bool read_asap() const override {
        return false;
      };
  };
  static void to_json(json &j, const FairLevelsPrice &k) {
    j = k.to_json();
  };

  struct Stdev {
    Price fv,
          topBid,
          topAsk;
  };
  static void to_json(json &j, const Stdev &k) {
    j = {
      { "fv", k.fv    },
      {"bid", k.topBid},
      {"ask", k.topAsk}
    };
  };
  static void from_json(const json &j, Stdev &k) {
    k.fv     = j.value("fv",  0.0);
    k.topBid = j.value("bid", 0.0);
    k.topAsk = j.value("ask", 0.0);
  };

  struct Stdevs: public Sqlite::VectorBackup<Stdev> {
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    private_ref:
      const Price         &fairValue;
      const QuotingParams &qp;
    public:
      Stdevs(const KryptoNinja &bot, const Price &f, const QuotingParams &q)
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
      mMatter about() const override {
        return mMatter::STDEVStats;
      };
      double limit() const override {
        return qp.quotingStdevProtectionPeriods;
      };
      Clock lifetime() const override {
        return 1e+3 * limit();
      };
    private:
      double calc(Price *const mean, const string &type) const {
        vector<Price> values;
        for (const Stdev &it : rows)
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
  static void to_json(json &j, const Stdevs &k) {
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
      mMatter about() const override {
        return mMatter::MarketDataLongTerm;
      };
      double limit() const override {
        return 5760;
      };
      Clock lifetime() const override {
        return 60e+3 * limit();
      };
    private:
      string explainOK() const override {
        return "loaded % historical Fair Values";
      };
  };

  struct Ewma: public Sqlite::StructBackup<Ewma>,
               public Client::Clicked {
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
      const KryptoNinja   &K;
      const Price         &fairValue;
      const QuotingParams &qp;
    public:
      Ewma(const KryptoNinja &bot, const Price &f, const QuotingParams &q)
        : StructBackup(bot)
        , Clicked(bot, {
            {&q, [&]() { calcFromHistory(); }}
          })
        , fairValue96h(bot)
        , K(bot)
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
      mMatter about() const override {
        return mMatter::EWMAStats;
      };
      Clock lifetime() const override {
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
        *mean = fairValue96h.at(x);
        while (n--) calc(mean, periods, fairValue96h.at(++x));
        K.log("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
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
        unsigned int max3size = fmin(3, fairValue96h.size());
        Price SMA3 = accumulate(fairValue96h.end() - max3size, fairValue96h.end(), Price(),
          [](Price sma3, const Price &it) { return sma3 + it; }
        ) / max3size;
        double targetPosition = 0;
        if (qp.autoPositionMode == AutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 1e+2 / mgEwmaL) - 1e+2);
          double newEwmacrossing = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2);
          targetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == AutoPositionMode::EWMA_LS)
          targetPosition = ((mgEwmaS * 1e+2 / mgEwmaL) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == AutoPositionMode::EWMA_4) {
          if (mgEwmaL < mgEwmaVL) targetPosition = -1;
          else targetPosition = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        }
        targetPositionAutoPercentage = ((1 + max(-1.0, min(1.0, targetPosition))) / 2) * 1e+2;
      };
      string explain() const override {
        return "EWMA Values";
      };
      string explainKO() const override {
        return "consider to warm up some %";
      };
  };
  static void to_json(json &j, const Ewma &k) {
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
  static void from_json(const json &j, Ewma &k) {
    k.mgEwmaVL = j.value("ewmaVeryLong",   0.0);
    k.mgEwmaL  = j.value("ewmaLong",       0.0);
    k.mgEwmaM  = j.value("ewmaMedium",     0.0);
    k.mgEwmaS  = j.value("ewmaShort",      0.0);
    k.mgEwmaXS = j.value("ewmaExtraShort", 0.0);
    k.mgEwmaU  = j.value("ewmaUltraShort", 0.0);
  };

  struct MarketStats: public Client::Broadcast<MarketStats> {
            Ewma ewma;
          Stdevs stdev;
    MarketTakers takerTrades;
    public:
      MarketStats(const KryptoNinja &bot, const Price &f, const QuotingParams &q)
        : Broadcast(bot)
        , ewma(bot, f, q)
        , stdev(bot, f, q)
        , takerTrades(bot)
      {};
      mMatter about() const override {
        return mMatter::MarketChart;
      };
      bool realtime() const override {
        return false;
      };
  };
  static void to_json(json &j, const MarketStats &k) {
    j = {
      {          "ewma", k.ewma                         },
      {    "stdevWidth", k.stdev                        },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
    };
  };

  struct LevelsDiff: public Levels,
                     public Client::Broadcast<LevelsDiff> {
      bool patched = false;
    private_ref:
      const Levels        &unfiltered;
      const QuotingParams &qp;
    public:
      LevelsDiff(const KryptoNinja &bot, const Levels &u, const QuotingParams &q)
        : Broadcast(bot)
        , unfiltered(u)
        , qp(q)
      {};
      bool empty() const {
        return patched
          ? bids.empty() and asks.empty()
          : bids.empty() or asks.empty();
      };
      void send_patch() {
        const bool full = empty();
        if (full) unfilter();
        else {
          if (ratelimit()) return;
          diff();
        }
        if (!empty() and read) read();
        if (!full) unfilter();
      };
      mMatter about() const override {
        return mMatter::MarketData;
      };
      json hello() override {
        unfilter();
        return Broadcast::hello();
      };
    private:
      bool ratelimit() {
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
      vector<Level> diff(const vector<Level> &from, vector<Level> to) const {
        vector<Level> patch;
        for (const Level &it : from) {
          auto it_ = find_if(
            to.begin(), to.end(),
            [&](const Level &_it) {
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
  static void to_json(json &j, const LevelsDiff &k) {
    to_json(j, (Levels)k);
    if (k.patched)
      j["diff"] = true;
  };
  struct MarketLevels: public Levels {
       unsigned int averageCount = 0;
              Price averageWidth = 0,
                    fairValue    = 0;
             Levels unfiltered;
         LevelsDiff diff;
        MarketStats stats;
    FairLevelsPrice fairPrice;
    private:
      unordered_map<Price, Amount> filterBidOrders,
                                   filterAskOrders;
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
      const Orders        &orders;
    public:
      MarketLevels(const KryptoNinja &bot, const QuotingParams &q, const Orders &o)
        : diff(bot, unfiltered, q)
        , stats(bot, fairValue, q)
        , fairPrice(bot, fairValue)
        , K(bot)
        , qp(q)
        , orders(o)
      {};
      void timer_1s() {
        stats.stdev.timer_1s(bids.cbegin()->price, asks.cbegin()->price);
      };
      void timer_60s() {
        stats.takerTrades.timer_60s();
        stats.ewma.timer_60s(resetAverageWidth());
        stats.broadcast();
      };
      Price calcQuotesWidth(bool *const superSpread) const {
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
      bool ready() {
        filter();
        if (!fairValue and Tspent > 21e+3)
          K.logWar("QE", "Unable to calculate quote, missing market data", 10e+3);
        return fairValue;
      };
      void read_from_gw(const Levels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
        if (fairPrice.broadcast()) K.repaint();
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
      Price resetAverageWidth() {
        averageCount = 0;
        return averageWidth;
      };
      void calcFairValue() {
        if (bids.empty() or asks.empty())
          fairValue = 0;
        else if (qp.fvModel == FairValueModel::BBO)
          fairValue = (asks.cbegin()->price
                     + bids.cbegin()->price) / 2;
        else if (qp.fvModel == FairValueModel::wBBO)
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
          fairValue = K.gateway->decimal.price.round(fairValue);
      };
      vector<Level> filter(vector<Level> levels, unordered_map<Price, Amount> *const filterOrders) {
        if (!filterOrders->empty())
          for (auto it = levels.begin(); it != levels.end();) {
            for (auto it_ = filterOrders->begin(); it_ != filterOrders->end();)
              if (abs(it->price - it_->first) < K.gateway->tickPrice) {
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

  struct Profit {
    Amount baseValue,
           quoteValue;
     Clock time;
  };
  static void to_json(json &j, const Profit &k) {
    j = {
      { "baseValue", k.baseValue },
      {"quoteValue", k.quoteValue},
      {      "time", k.time      }
     };
  };
  static void from_json(const json &j, Profit &k) {
    k.baseValue  = j.value("baseValue",  0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time",  (Clock)0);
  };
  struct Profits: public Sqlite::VectorBackup<Profit> {
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
    public:
      Profits(const KryptoNinja &bot, const QuotingParams &q)
        : VectorBackup(bot)
        , K(bot)
        , qp(q)
      {};
      bool ratelimit() const {
        return !empty() and crbegin()->time + 21e+3 > Tstamp;
      };
      double calcBaseDiff() const {
        return calcDiffPercent(
          cbegin()->baseValue,
          crbegin()->baseValue
        );
      };
      double calcQuoteDiff() const {
        return calcDiffPercent(
          cbegin()->quoteValue,
          crbegin()->quoteValue
        );
      };
      double calcDiffPercent(Amount older, Amount newer) const {
        return K.gateway->decimal.percent.round(((newer - older) / (older?:1)) * 1e+2);
      };
      mMatter about() const override {
        return mMatter::Profit;
      };
      void erase() override {
        const Clock now = Tstamp;
        for (auto it = begin(); it != end();)
          if (it->time + lifetime() > now) ++it;
          else it = rows.erase(it);
      };
      double limit() const override {
        return qp.profitHourInterval;
      };
      Clock lifetime() const override {
        return 3600e+3 * limit();
      };
    private:
      string explainOK() const override {
        return "loaded % historical Profits";
      };
  };

  struct OrderFilled: public Trade {
     string tradeId;
     Amount value,
            feeCharged,
            Kqty,
            Kvalue,
            delta;
      Price Kprice;
      Clock Ktime;
       bool isPong,
            loadedFromDB;
  };
  static void to_json(json &j, const OrderFilled &k) {
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
      {       "delta", k.delta       },
      {  "feeCharged", k.feeCharged  },
      {      "isPong", k.isPong      },
      {"loadedFromDB", k.loadedFromDB}
    };
  };
  static void from_json(const json &j, OrderFilled &k) {
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
    k.delta        = j.value("delta",
                       j.value("Kdiff",    0.0)
                     );
    k.feeCharged   = j.value("feeCharged", 0.0);
    k.isPong       = j.value("isPong",   false);
    k.loadedFromDB = true;
  };

  struct ButtonSubmitNewOrder: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonSubmitNewOrder(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if (j.is_object() and j.value("price", 0.0) and j.value("quantity", 0.0)) {
          json order = j;
          order["manual"]  = true;
          order["orderId"] = K.gateway->randId();
          K.clicked(this, order);
        }
      };
      mMatter about() const override {
        return mMatter::SubmitNewOrder;
      };
  };
  struct ButtonCancelOrder: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonCancelOrder(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if ((j.is_object() and !j.value("orderId", "").empty()))
          K.clicked(this, j.at("orderId").get<string>());
      };
      mMatter about() const override {
        return mMatter::CancelOrder;
      };
  };
  struct ButtonCancelAllOrders: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonCancelAllOrders(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json&) override {
        K.clicked(this);
      };
      mMatter about() const override {
        return mMatter::CancelAllOrders;
      };
  };
  struct ButtonCleanAllClosedTrades: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonCleanAllClosedTrades(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json&) override {
        K.clicked(this);
      };
      mMatter about() const override {
        return mMatter::CleanAllClosedTrades;
      };
  };
  struct ButtonCleanAllTrades: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonCleanAllTrades(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json&) override {
        K.clicked(this);
      };
      mMatter about() const override {
        return mMatter::CleanAllTrades;
      };
  };
  struct ButtonCleanTrade: public Client::Clickable {
    private_ref:
      const KryptoNinja &K;
    public:
      ButtonCleanTrade(const KryptoNinja &bot)
        : Clickable(bot)
        , K(bot)
      {};
      void click(const json &j) override {
        if ((j.is_object() and !j.value("tradeId", "").empty()))
          K.clicked(this, j.at("tradeId").get<string>());
      };
      mMatter about() const override {
        return mMatter::CleanTrade;
      };
  };
  struct Notepad: public Client::Broadcast<Notepad>,
                  public Client::Clickable {
    public:
      string content;
    public:
      Notepad(const KryptoNinja &bot)
        : Broadcast(bot)
        , Clickable(bot)
      {};
      void click(const json &j) override {
        if (j.is_array() and j.size() and j.at(0).is_string())
         content = j.at(0);
      };
      mMatter about() const override {
        return mMatter::Notepad;
      };
  };
  static void to_json(json &j, const Notepad &k) {
    j = k.content;
  };

  struct Buttons {
    Notepad                    notepad;
    ButtonSubmitNewOrder       submit;
    ButtonCancelOrder          cancel;
    ButtonCancelAllOrders      cancelAll;
    ButtonCleanAllClosedTrades cleanTradesClosed;
    ButtonCleanAllTrades       cleanTrades;
    ButtonCleanTrade           cleanTrade;
    Buttons(const KryptoNinja &bot)
      : notepad(bot)
      , submit(bot)
      , cancel(bot)
      , cancelAll(bot)
      , cleanTradesClosed(bot)
      , cleanTrades(bot)
      , cleanTrade(bot)
    {};
  };

  struct TradesHistory: public Sqlite::VectorBackup<OrderFilled>,
                        public Client::Broadcast<OrderFilled>,
                        public Client::Clicked {
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
    public:
      TradesHistory(const KryptoNinja &bot, const QuotingParams &q, const Buttons &b)
        : VectorBackup(bot)
        , Broadcast(bot)
        , Clicked(bot, {
            {&b.cleanTrade, [&](const json &j) { clearOne(j); }},
            {&b.cleanTrades, [&]() { clearAll(); }},
            {&b.cleanTradesClosed, [&]() { clearClosed(); }}
          })
        , K(bot)
        , qp(q)
      {};
      void insert(const LastOrder &order) {
        const Amount fee = 0;
        const Clock time = Tstamp;
        OrderFilled filled = {
          order.side,
          order.price,
          order.filled,
          time,
          to_string(time),
          K.gateway->margin == Future::Spot
            ? abs(order.price * order.filled)
            : order.filled,
          fee,
          0, 0, 0, 0, 0,
          order.isPong,
          false
        };
        const bool is_bid = order.side == Side::Bid;
        K.log("GW " + K.gateway->exchange, string(order.isPong?"PONG":"PING") + " TRADE "
          + (is_bid ? "BUY  " : "SELL ")
          + K.gateway->decimal.amount.str(filled.quantity) + ' '
          + (K.gateway->margin == Future::Spot ? K.gateway->base : "Contracts") + " at price "
          + K.gateway->decimal.price.str(filled.price) + ' ' + K.gateway->quote + " (value "
          + K.gateway->decimal.price.str(filled.value) + ' ' + K.gateway->quote + ")"
        );
        if (qp.safety == QuotingSafety::Off
          or qp.safety == QuotingSafety::PingPong
          or qp.safety == QuotingSafety::PingPoing
        ) broadcast_push_back(filled);
        else {
          Price widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * filled.price / 100
            : qp.widthPong;
          map<Price, string> matches;
          for (OrderFilled &it : rows)
            if (it.quantity - it.Kqty > 0 and it.side != filled.side) {
              const Price combinedFee = K.gateway->makeFee * (it.price + filled.price);
              if (is_bid
                  ? (it.price > filled.price + widthPong + combinedFee)
                  : (it.price < filled.price - widthPong - combinedFee)
              ) matches[it.price] = it.tradeId;
            }
          matchPong(
            matches,
            filled,
            (qp.pongAt == PongAt::LongPingFair or qp.pongAt == PongAt::LongPingAggressive)
              ? !is_bid
              : is_bid
          );
        }
        if (qp.cleanPongsAuto)
          clearPongsAuto();
      };
      mMatter about() const override {
        return mMatter::Trades;
      };
      void erase() override {
        if (crbegin()->Kqty < 0) rows.pop_back();
      };
      json blob() const override {
        if (crbegin()->Kqty == -1) return nullptr;
        else return VectorBackup::blob();
      };
      string increment() const override {
        return crbegin()->tradeId;
      };
      json hello() override {
        for (OrderFilled &it : rows)
          it.loadedFromDB = true;
        return rows;
      };
    private:
      void clearAll() {
        clear_if([](iterator) {
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
      void clear_if(const function<bool(iterator)> &fn, const bool &onlyOne = false) {
        for (auto it = begin(); it != end();)
          if (fn(it)) {
            it->Kqty = -1;
            it = send_push_erase(it);
            if (onlyOne) break;
          } else ++it;
      };
      void matchPong(const map<Price, string> &matches, OrderFilled pong, const bool &reverse) {
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
      bool matchPong(const string &match, OrderFilled *const pong) {
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
            it->delta = ((it->quantity * it->price) - (it->Kqty * it->Kprice))
                      * (it->side == Side::Ask ? 1 : -1);
          it->isPong = true;
          it->loadedFromDB = false;
          it = send_push_erase(it);
          break;
        }
        return pong->quantity > 0;
      };
      void broadcast_push_back(const OrderFilled &row) {
        rows.push_back(row);
        backup();
        if (crbegin()->Kqty < 0) rbegin()->Kqty = -2;
        broadcast();
      };
      iterator send_push_erase(iterator it) {
        OrderFilled row = *it;
        it = rows.erase(it);
        broadcast_push_back(row);
        erase();
        return it;
      };
      string explainOK() const override {
        return "loaded % historical Trades";
      };
  };

  struct RecentTrade {
     Price price    = 0;
    Amount quantity = 0;
     Clock time     = 0;
    RecentTrade(const Price &p, const Amount &q)
      : price(p)
      , quantity(q)
      , time(Tstamp)
    {};
  };
  struct RecentTrades {
    private_ref:
      const QuotingParams &qp;
    public:
      RecentTrades(const QuotingParams &q)
        : qp(q)
      {};
    multimap<Price, RecentTrade> buys,
                                 sells;
                          Amount sumBuys       = 0,
                                 sumSells      = 0;
                           Price lastBuyPrice  = 0,
                                 lastSellPrice = 0;
    void insert(const LastOrder &order) {
      (order.side == Side::Bid
        ? lastBuyPrice
        : lastSellPrice
      ) = order.price;
      (order.side == Side::Bid
        ? buys
        : sells
      ).insert(pair<Price, RecentTrade>(
        order.price,
        RecentTrade(order.price, order.filled)
      ));
    };
    void expire() {
      if (!buys.empty()) expire(&buys);
      if (!sells.empty()) expire(&sells);
      skip();
      sumBuys = sum(&buys);
      sumSells = sum(&sells);
    };
    private:
      Amount sum(multimap<Price, RecentTrade> *const k) const {
        Amount sum = 0;
        for (const auto &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void expire(multimap<Price, RecentTrade> *const k) {
        const Clock now = Tstamp;
        for (auto it = k->begin(); it != k->end();)
          if (it->second.time + qp.tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (!(buys.empty() or sells.empty())) {
          RecentTrade &buy = buys.rbegin()->second;
          RecentTrade &sell = sells.begin()->second;
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

  struct Safety: public Client::Broadcast<Safety> {
             double buy      = 0,
                    sell     = 0,
                    combined = 0;
              Price buyPing  = 0,
                    sellPing = 0;
             Amount buySize  = 0,
                    sellSize = 0;
      TradesHistory trades;
       RecentTrades recentTrades;
    private_ref:
      const QuotingParams &qp;
      const Wallet        &base;
      const Price         &fairValue;
      const Amount        &targetBasePosition;
      const Amount        &positionDivergence;
    public:
      Safety(const KryptoNinja &bot, const QuotingParams &q, const Wallet &w, const Buttons &b, const Price &f, const Amount &t, const Amount &p)
        : Broadcast(bot)
        , trades(bot, q, b)
        , recentTrades(q)
        , qp(q)
        , base(w)
        , fairValue(f)
        , targetBasePosition(t)
        , positionDivergence(p)
      {};
      void timer_1s() {
        calc();
      };
      void insertTrade(const LastOrder &order) {
        if (!order.isPong)
          recentTrades.insert(order);
        trades.insert(order);
        calc();
      };
      void calc() {
        if (!base.value or !fairValue) return;
        calcSizes();
        calcPrices();
        recentTrades.expire();
        if (empty()) return;
        buy  = recentTrades.sumBuys / buySize;
        sell = recentTrades.sumSells / sellSize;
        combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
        broadcast();
      };
      bool empty() const {
        return !base.value or !buySize or !sellSize;
      };
      mMatter about() const override {
        return mMatter::TradeSafetyValue;
      };
      bool read_same_blob() const override {
        return false;
      };
    private:
      void calcSizes() {
        if (qp.percentageValues) {
          sellSize = qp.sellSizePercentage / 1e+2;
          buySize  = qp.buySizePercentage / 1e+2;
          const Amount pdivMin = fmax(0, targetBasePosition - positionDivergence),
                       pdivMax = fmin(base.value, targetBasePosition + positionDivergence);
          if (qp.orderPctTotal == OrderPctTotal::Side) {
            sellSize *= base.total;
            buySize  *= base.value - base.total;
          } else {
            if (qp.orderPctTotal == OrderPctTotal::TBPSide) {
              sellSize *= pow((base.total - pdivMin) / (base.value - pdivMin), qp.tradeSizeTBPExp);
              buySize  *= pow((pdivMax - base.total) / pdivMax, qp.tradeSizeTBPExp);
            }
            sellSize *= base.value;
            buySize  *= base.value;
            if (qp.orderPctTotal == OrderPctTotal::TBPSide
              or qp.orderPctTotal == OrderPctTotal::TBPValue
            ) {
              const double exp = qp.orderPctTotal == OrderPctTotal::TBPValue
                               ? 1.0
                               : qp.tradeSizeTBPExp;
              if (targetBasePosition * 2 < base.value)
                buySize *= pow(
                  (targetBasePosition - pdivMin) * pdivMax / ((base.value - pdivMin) * (pdivMax - targetBasePosition)),
                  exp
                );
              else
                sellSize *= pow(
                  (base.value - pdivMin) * (pdivMax - targetBasePosition) / ((targetBasePosition - pdivMin) * pdivMax),
                  exp
                );
            }
          }
          sellSize = fmax(0.0, sellSize);
          buySize  = fmax(0.0, buySize);
        } else {
          sellSize = qp.sellSize;
          buySize  = qp.buySize;
        }
        if (qp.aggressivePositionRebalancing == APR::Off) return;
        if (qp.buySizeMax)
          buySize = fmax(buySize, targetBasePosition - base.total);
        if (qp.sellSizeMax)
          sellSize = fmax(sellSize, base.total - targetBasePosition);
      };
      void calcPrices() {
        if (qp.safety == QuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
          if (qp.safety == QuotingSafety::Off) return;
          Price widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * fairValue / 100
            : qp.widthPong;
          if (qp.safety == QuotingSafety::PingPoing) {
            if (recentTrades.lastBuyPrice and fairValue > recentTrades.lastBuyPrice - widthPong)
              buyPing = recentTrades.lastBuyPrice;
            if (recentTrades.lastSellPrice and fairValue < recentTrades.lastSellPrice + widthPong)
              sellPing = recentTrades.lastSellPrice;
          } else {
            map<Price, OrderFilled> tradesBuy;
            map<Price, OrderFilled> tradesSell;
            for (const OrderFilled &it: trades)
              (it.side == Side::Bid ? tradesBuy : tradesSell)[it.price] = it;
            Amount buyQty = 0,
                   sellQty = 0;
            if (qp.pongAt == PongAt::ShortPingFair or qp.pongAt == PongAt::ShortPingAggressive) {
              matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
              matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
              if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
              if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
            } else if (qp.pongAt == PongAt::LongPingFair or qp.pongAt == PongAt::LongPingAggressive) {
              matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
              matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
            } else if (qp.pongAt == PongAt::AveragePingFair or qp.pongAt == PongAt::AveragePingAggressive) {
              matchAllPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
              matchAllPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
            }
            if (buyQty) buyPing /= buyQty;
            if (sellQty) sellPing /= sellQty;
          }
        }
      };
      void matchFirstPing(map<Price, OrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<Price, OrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, false, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<Price, OrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(false, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<Price, OrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width) {
        matchPing(false, false, tradesSide, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<Price, OrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (auto it = tradesSide->crbegin(); it != tradesSide->crend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (const auto &it : *tradesSide)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it.second.price, it.second.quantity, it.second.price, it.second.Kqty, reverse))
            break;
      };
      bool matchPing(bool _near, bool _far, Price *ping, Amount *qty, Amount qtyMax, Price width, Price fv, Price price, Amount qtyTrade, Price priceTrade, Amount KqtyTrade, bool reverse) {
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
  static void to_json(json &j, const Safety &k) {
    j = {
      {     "buy", k.buy     },
      {    "sell", k.sell    },
      {"combined", k.combined},
      { "buyPing", k.buyPing },
      {"sellPing", k.sellPing}
    };
  };

  struct Target: public Sqlite::StructBackup<Target>,
                 public Client::Broadcast<Target> {
    Amount targetBasePosition = 0,
           positionDivergence = 0;
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
      const double        &targetPositionAutoPercentage;
      const Amount        &baseValue;
    public:
      Target(const KryptoNinja &bot, const QuotingParams &q, const double &t, const Amount &v)
        : StructBackup(bot)
        , Broadcast(bot)
        , K(bot)
        , qp(q)
        , targetPositionAutoPercentage(t)
        , baseValue(v)
      {};
      void calcTargetBasePos() {
        if (!baseValue) {
          if (Tspent > 21e+3)
            K.logWar("QE", "Unable to calculate TBP, missing wallet data", 3e+3);
          return;
        }
        targetBasePosition = K.gateway->decimal.funds.round(
          qp.autoPositionMode == AutoPositionMode::Manual
            ? (qp.percentageValues
              ? qp.targetBasePositionPercentage * baseValue / 1e+2
              : qp.targetBasePosition)
            : (qp.percentageValues
              ? ( qp.targetBasePositionPercentageMin + ( qp.targetBasePositionPercentageMax - qp.targetBasePositionPercentageMin ) * targetPositionAutoPercentage / 1e+2 ) / 1e+2 * baseValue
              : ( qp.targetBasePositionMin + ( qp.targetBasePositionMax - qp.targetBasePositionMin ) * targetPositionAutoPercentage / 1e+2 ))
        );
        calcPDiv();
        if (broadcast()) {
          backup();
          if (K.arg<int>("debug-wallet")) report();
        }
      };
      bool realtime() const override {
        return false;
      };
      mMatter about() const override {
        return mMatter::TargetBasePosition;
      };
      bool read_same_blob() const override {
        return false;
      };
    private:
      void calcPDiv() {
        Amount pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * baseValue / 1e+2
          : qp.positionDivergence;
        if (qp.autoPositionMode == AutoPositionMode::Manual or PDivMode::Manual == qp.positionDivergenceMode)
          positionDivergence = pDiv;
        else {
          Amount pDivMin = qp.percentageValues
            ? qp.positionDivergencePercentageMin * baseValue / 1e+2
            : qp.positionDivergenceMin;
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          if (PDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (PDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (PDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (PDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
        positionDivergence = K.gateway->decimal.funds.round(positionDivergence);
      };
      void report() const {
        K.log("QE", "TBP: "
          + to_string((int)(targetBasePosition / baseValue * 1e+2)) + "% = " + K.gateway->decimal.funds.str(targetBasePosition)
          + " " + K.gateway->base + ", pDiv: "
          + to_string((int)(positionDivergence / baseValue * 1e+2)) + "% = " + K.gateway->decimal.funds.str(positionDivergence)
          + " " + K.gateway->base);
      };
      string explain() const override {
        return to_string(targetBasePosition);
      };
      string explainOK() const override {
        return "loaded TBP = % " + K.gateway->base;
      };
  };
  static void to_json(json &j, const Target &k) {
    j = {
      { "tbp", k.targetBasePosition},
      {"pDiv", k.positionDivergence}
    };
  };
  static void from_json(const json &j, Target &k) {
    k.targetBasePosition = j.value("tbp",  0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
  };

  struct Wallets: public Client::Broadcast<Wallets> {
     Wallet base,
            quote;
     Target target;
     Safety safety;
    Profits profits;
    private_ref:
      const KryptoNinja &K;
      const Orders      &orders;
      const Price       &fairValue;
    public:
      Wallets(const KryptoNinja &bot, const QuotingParams &q, const Orders &o, const Buttons &b, const MarketLevels &l)
        : Broadcast(bot)
        , target(bot, q, l.stats.ewma.targetPositionAutoPercentage, base.value)
        , safety(bot, q, base, b, l.fairValue, target.targetBasePosition, target.positionDivergence)
        , profits(bot, q)
        , K(bot)
        , orders(o)
        , fairValue(l.fairValue)
      {};
      bool ready() const {
        return !safety.empty();
      };
      void read_from_gw(const Wallet &raw) {
        if      (raw.currency == K.gateway->base)  base  = raw;
        else if (raw.currency == K.gateway->quote) quote = raw;
        if (base.currency.empty() or quote.currency.empty() or !fairValue) return;
        calcMaxFunds();
        calcFunds();
      };
      void calcFunds() {
        calcFundsSilently();
        broadcast();
      };
      void calcFundsAfterOrder() {
        if (!orders.updated.price) return;
        if (K.gateway->margin == Future::Spot) {
          calcHeldAmount(orders.updated.side);
          calcFundsSilently();
        }
        if (orders.updated.filled) {
          safety.insertTrade(orders.updated);
          K.gateway->balance();
        }
      };
      mMatter about() const override {
        return mMatter::Position;
      };
      bool realtime() const override {
        return false;
      };
      bool read_asap() const override {
        return false;
      };
      bool read_same_blob() const override {
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
        if (side == Side::Ask)
          base = {base.total - heldSide, heldSide, base.currency};
        else if (side == Side::Bid)
          quote = {quote.total - heldSide, heldSide, quote.currency};
      };
      void calcValues() {
        base.value  = K.gateway->margin == Future::Spot
                        ? base.total + (quote.total / fairValue)
                        : base.total;
        quote.value = K.gateway->margin == Future::Spot
                        ? (base.total * fairValue) + quote.total
                        : (K.gateway->margin == Future::Inverse
                            ? base.total * fairValue
                            : base.total / fairValue
                        );
      };
      void calcProfits() {
        if (!profits.ratelimit())
          profits.push_back({base.value, quote.value, Tstamp});
        base.profit  = profits.calcBaseDiff();
        quote.profit = profits.calcQuoteDiff();
      };
      void calcMaxFunds() {
        auto limit = K.arg<double>("wallet-limit");
        if (limit) {
          limit -= quote.held / fairValue;
          if (limit > 0 and quote.amount / fairValue > limit) {
            quote.amount = limit * fairValue;
            base.amount = limit = 0;
          } else limit -= quote.amount / fairValue;
          limit -= base.held;
          if (limit > 0 and base.amount > limit)
            base.amount = limit;
        }
      };
  };
  static void to_json(json &j, const Wallets &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };

  struct Quote: public Level {
    const Side       side   = (Side)0;
          QuoteState state  = QuoteState::MissingData;
          bool       isPong = false;
    Quote(const Side &s)
      : side(s)
    {};
    bool empty() const {
      return !size or !price;
    };
    void skip() {
      size = 0;
    };
    void clear(const QuoteState &reason) {
      price = size = 0;
      state = reason;
    };
    virtual bool deprecates(const Price&) const = 0;
    bool checkCrossed(const Quote &opposite) {
      if (empty()) return false;
      if (opposite.empty() or deprecates(opposite.price)) {
        state = QuoteState::Live;
        return false;
      }
      state = QuoteState::Crossed;
      return true;
    };
  };
  struct QuoteBid: public Quote {
    QuoteBid()
      : Quote(Side::Bid)
    {};
    bool deprecates(const Price &higher) const override {
      return price < higher;
    };
  };
  struct QuoteAsk: public Quote {
    QuoteAsk()
      : Quote(Side::Ask)
    {};
    bool deprecates(const Price &lower) const override {
      return price > lower;
    };
  };
  struct Quotes {
    QuoteBid bid;
    QuoteAsk ask;
        bool superSpread = false;
    private_ref:
      const KryptoNinja &K;
    public:
      Quotes(const KryptoNinja &bot)
        : K(bot)
      {};
      void checkCrossedQuotes() {
        if (bid.checkCrossed(ask) or ask.checkCrossed(bid))
          K.logWar("QE", "Crossed bid/ask quotes detected, that is.. unexpected", 3e+3);
      };
      void debug(const string &step) {
        if (K.arg<int>("debug-quotes"))
          K.log("DEBUG QE", "[" + step + "] "
            + to_string((int)bid.state) + ":"
            + to_string((int)ask.state) + " "
            + ((json){{"bid", bid}, {"ask", ask}}).dump()
          );
      };
  };

  struct DummyMarketMaker: public Client::Clicked {
    private:
      void (*calcRawQuotesFromMarket)(
        const MarketLevels&,
        const Price&,
        const Price&,
        const Amount&,
        const Amount&,
              Quotes&
      ) = nullptr;
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
      const MarketLevels  &levels;
      const Wallets       &wallet;
            Quotes        &quotes;
    public:
      DummyMarketMaker(const KryptoNinja &bot, const QuotingParams &q, const MarketLevels &l, const Wallets &w, Quotes &Q)
        : Clicked(bot, {
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
          K.gateway->tickPrice,
          levels.calcQuotesWidth(&quotes.superSpread),
          wallet.safety.buySize,
          wallet.safety.sellSize,
          quotes
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(QuoteState::WidthTooHigh);
          quotes.ask.clear(QuoteState::WidthTooHigh);
          K.logWar("QP", "Negative price detected, widthPing must be lower", 3e+3);
        }
      };
    private:
      void mode() {
        if (qp.mode == QuotingMode::Top)              calcRawQuotesFromMarket = calcTopOfMarket;
        else if (qp.mode == QuotingMode::Mid)         calcRawQuotesFromMarket = calcMidOfMarket;
        else if (qp.mode == QuotingMode::Join)        calcRawQuotesFromMarket = calcJoinMarket;
        else if (qp.mode == QuotingMode::InverseJoin) calcRawQuotesFromMarket = calcInverseJoinMarket;
        else if (qp.mode == QuotingMode::InverseTop)  calcRawQuotesFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == QuotingMode::HamelinRat)  calcRawQuotesFromMarket = calcColossusOfMarket;
        else if (qp.mode == QuotingMode::Depth)       calcRawQuotesFromMarket = calcDepthOfMarket;
        else error("QE", "Invalid quoting mode saved, consider to remove the database file");
      };
      static void quoteAtTopOfMarket(const MarketLevels &levels, const Price &tickPrice, Quotes &quotes) {
        const Level &topBid = levels.bids.begin()->size > tickPrice
          ? levels.bids.at(0)
          : levels.bids.at(levels.bids.size() > 1);
        const Level &topAsk = levels.asks.begin()->size > tickPrice
          ? levels.asks.at(0)
          : levels.asks.at(levels.asks.size() > 1);
        quotes.bid.price = topBid.price;
        quotes.ask.price = topAsk.price;
      };
      static void calcTopOfMarket(
        const MarketLevels &levels,
        const Price        &tickPrice,
        const Price        &widthPing,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, tickPrice, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price + tickPrice);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price - tickPrice);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcMidOfMarket(
        const MarketLevels &levels,
        const Price        &,
        const Price        &widthPing,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quotes.bid.price = fmax(levels.fairValue - widthPing, 0);
        quotes.ask.price = levels.fairValue + widthPing;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcJoinMarket(
        const MarketLevels &levels,
        const Price        &tickPrice,
        const Price        &widthPing,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, tickPrice, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseJoinMarket(
        const MarketLevels &levels,
        const Price        &tickPrice,
        const Price        &widthPing,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, tickPrice, quotes);
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
        const MarketLevels &levels,
        const Price        &tickPrice,
        const Price        &widthPing,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, tickPrice, quotes);
        Price mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        quotes.bid.price = quotes.bid.price + tickPrice;
        quotes.ask.price = quotes.ask.price - tickPrice;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcColossusOfMarket(
        const MarketLevels &levels,
        const Price        &tickPrice,
        const Price        &,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, tickPrice, quotes);
        quotes.bid.size = 0;
        quotes.ask.size = 0;
        for (const Level &it : levels.bids)
          if (quotes.bid.size < it.size and it.price <= quotes.bid.price) {
            quotes.bid.size = it.size;
            quotes.bid.price = it.price;
          }
        for (const Level &it : levels.asks)
          if (quotes.ask.size < it.size and it.price >= quotes.ask.price) {
            quotes.ask.size = it.size;
            quotes.ask.price = it.price;
          }
        if (quotes.bid.size) quotes.bid.price += tickPrice;
        if (quotes.ask.size) quotes.ask.price -= tickPrice;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcDepthOfMarket(
        const MarketLevels &levels,
        const Price        &,
        const Price        &depth,
        const Amount       &bidSize,
        const Amount       &askSize,
              Quotes       &quotes
      ) {
        Price bidPx = levels.bids.cbegin()->price;
        Amount bidDepth = 0;
        for (const Level &it : levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        Price askPx = levels.asks.cbegin()->price;
        Amount askDepth = 0;
        for (const Level &it : levels.asks) {
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

  struct AntonioCalculon: public Client::Broadcast<AntonioCalculon> {
                   Quotes quotes;
         DummyMarketMaker dummyMM;
     vector<const Order*> zombies;
             unsigned int countWaiting = 0,
                          countWorking = 0,
                          AK47inc      = 0;
                 SideAPR  sideAPR      = SideAPR::Off;
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
      const MarketLevels  &levels;
      const Wallets       &wallet;
    public:
      AntonioCalculon(const KryptoNinja &bot, const QuotingParams &q, const MarketLevels &l, const Wallets &w)
        : Broadcast(bot)
        , quotes(bot)
        , dummyMM(bot, q, l, w, quotes)
        , K(bot)
        , qp(q)
        , levels(l)
        , wallet(w)
      {};
      vector<const Order*> purge() {
        broadcast();
        countWaiting =
        countWorking = 0;
        vector<const Order*> zombies_;
        zombies.swap(zombies_);
        return zombies_;
      };
      void offline() {
        states(QuoteState::Disconnected);
      };
      void paused() {
        states(QuoteState::DisabledQuotes);
      };
      void calcQuotes() {
        states(QuoteState::UnknownHeld);
        dummyMM.calcRawQuotes();
        applyQuotingParameters();
      };
      bool abandon(const Order &order, Quote &quote, unsigned int &bullets) {
        if (stillAlive(order)) {
          if (abs(order.price - quote.price) < K.gateway->tickPrice)
            quote.skip();
          else if (order.status == Status::Waiting) {
            if (qp.safety != QuotingSafety::AK47
              or !--bullets
            ) quote.skip();
          } else if (qp.safety != QuotingSafety::AK47
            or quote.deprecates(order.price)
          ) {
            if (qp.lifetime and order.time + qp.lifetime > Tstamp)
              quote.skip();
            else return true;
          }
        }
        return false;
      };
      mMatter about() const override {
        return mMatter::QuoteStatus;
      };
      bool realtime() const override {
        return false;
      };
      bool read_same_blob() const override {
        return false;
      };
    private:
      void states(const QuoteState &state) {
        quotes.bid.state =
        quotes.ask.state = state;
      };
      bool stillAlive(const Order &order) {
        if (order.status == Status::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
          ++countWaiting;
        } else ++countWorking;
        return !order.manual;
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
          or (qp.superTrades != SOP::Size and qp.superTrades != SOP::TradesSize)
        ) return;
        if (!qp.buySizeMax and !quotes.bid.empty())
          quotes.bid.size = fmin(
            qp.sopSizeMultiplier * quotes.bid.size,
            K.gateway->margin == Future::Spot
              ? (wallet.quote.amount / quotes.bid.price) / 2
              : (K.gateway->margin == Future::Inverse
                  ? (wallet.base.amount / 2) * quotes.bid.price
                  : (wallet.base.amount / 2) / quotes.bid.price
              )
          );
        if (!qp.sellSizeMax and !quotes.ask.empty())
          quotes.ask.size = fmin(
            qp.sopSizeMultiplier * quotes.ask.size,
            K.gateway->margin == Future::Spot
              ? wallet.base.amount / 2
              : (K.gateway->margin == Future::Inverse
                  ? (wallet.base.amount / 2) * quotes.ask.price
                  : (wallet.base.amount / 2) / quotes.ask.price
              )
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
          quotes.ask.clear(QuoteState::TBPHeld);
          if (!quotes.bid.empty() and qp.aggressivePositionRebalancing != APR::Off) {
            sideAPR = SideAPR::Buy;
            if (!qp.buySizeMax)
              quotes.bid.size = fmin(
                qp.aprMultiplier * quotes.bid.size,
                K.gateway->margin == Future::Spot
                  ? wallet.target.targetBasePosition - wallet.base.total
                  : (K.gateway->margin == Future::Inverse
                      ? (wallet.target.targetBasePosition - wallet.base.total) * quotes.bid.price
                      : (wallet.target.targetBasePosition - wallet.base.total) / quotes.bid.price
                  )
              );
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          quotes.bid.clear(QuoteState::TBPHeld);
          if (!quotes.ask.empty() and qp.aggressivePositionRebalancing != APR::Off) {
            sideAPR = SideAPR::Sell;
            if (!qp.sellSizeMax)
              quotes.ask.size = fmin(
                qp.aprMultiplier * quotes.ask.size,
                K.gateway->margin == Future::Spot
                  ? wallet.base.total - wallet.target.targetBasePosition
                  : (K.gateway->margin == Future::Inverse
                      ? (wallet.base.total - wallet.target.targetBasePosition) * quotes.ask.price
                      : (wallet.base.total - wallet.target.targetBasePosition) / quotes.ask.price
                  )
              );
          }
        }
        else sideAPR = SideAPR::Off;
      };
      void applyStdevProtection() {
        if (qp.quotingStdevProtection == STDEV::Off or !levels.stats.stdev.fair) return;
        if (!quotes.ask.empty() and (
          qp.quotingStdevProtection == STDEV::OnFV
          or qp.quotingStdevProtection == STDEV::OnTops
          or qp.quotingStdevProtection == STDEV::OnTop
          or sideAPR != SideAPR::Sell
        ))
          quotes.ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == STDEV::OnFV or qp.quotingStdevProtection == STDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean
                : ((qp.quotingStdevProtection == STDEV::OnTops or qp.quotingStdevProtection == STDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean
                  : levels.stats.stdev.askMean)
              : levels.fairValue
            ) + ((qp.quotingStdevProtection == STDEV::OnFV or qp.quotingStdevProtection == STDEV::OnFVAPROff)
              ? levels.stats.stdev.fair
              : ((qp.quotingStdevProtection == STDEV::OnTops or qp.quotingStdevProtection == STDEV::OnTopsAPROff)
                ? levels.stats.stdev.top
                : levels.stats.stdev.ask)),
            quotes.ask.price
          );
        if (!quotes.bid.empty() and (
          qp.quotingStdevProtection == STDEV::OnFV
          or qp.quotingStdevProtection == STDEV::OnTops
          or qp.quotingStdevProtection == STDEV::OnTop
          or sideAPR != SideAPR::Buy
        ))
          quotes.bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == STDEV::OnFV or qp.quotingStdevProtection == STDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean
                : ((qp.quotingStdevProtection == STDEV::OnTops or qp.quotingStdevProtection == STDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean
                  : levels.stats.stdev.bidMean)
              : levels.fairValue
            ) - ((qp.quotingStdevProtection == STDEV::OnFV or qp.quotingStdevProtection == STDEV::OnFVAPROff)
              ? levels.stats.stdev.fair
              : ((qp.quotingStdevProtection == STDEV::OnTops or qp.quotingStdevProtection == STDEV::OnTopsAPROff)
                ? levels.stats.stdev.top
                : levels.stats.stdev.bid)),
            quotes.bid.price
          );
      };
      void applyAggressivePositionRebalancing() {
        if (qp.safety == QuotingSafety::Off) return;
        const Price widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        if (!quotes.ask.empty() and wallet.safety.buyPing) {
          const Price sellPong = (wallet.safety.buyPing * (1 + K.gateway->makeFee) + widthPong) / (1 - K.gateway->makeFee);
          if ((qp.aggressivePositionRebalancing == APR::SizeWidth and sideAPR == SideAPR::Sell)
            or ((qp.safety == QuotingSafety::PingPong or qp.safety == QuotingSafety::PingPoing)
              ? quotes.ask.price < sellPong
              : qp.pongAt == PongAt::ShortPingAggressive
                or qp.pongAt == PongAt::AveragePingAggressive
                or qp.pongAt == PongAt::LongPingAggressive
            )
          ) quotes.ask.price = fmax(levels.bids.at(0).price + K.gateway->tickPrice, sellPong);
          quotes.ask.isPong = quotes.ask.price >= sellPong;
        }
        if (!quotes.bid.empty() and wallet.safety.sellPing) {
          const Price buyPong = (wallet.safety.sellPing * (1 - K.gateway->makeFee) - widthPong) / (1 + K.gateway->makeFee);
          if ((qp.aggressivePositionRebalancing == APR::SizeWidth and sideAPR == SideAPR::Buy)
            or ((qp.safety == QuotingSafety::PingPong or qp.safety == QuotingSafety::PingPoing)
              ? quotes.bid.price > buyPong
              : qp.pongAt == PongAt::ShortPingAggressive
                or qp.pongAt == PongAt::AveragePingAggressive
                or qp.pongAt == PongAt::LongPingAggressive
            )
          ) quotes.bid.price = fmin(levels.asks.at(0).price - K.gateway->tickPrice, buyPong);
          quotes.bid.isPong = quotes.bid.price <= buyPong;
        }
      };
      void applyAK47Increment() {
        if (qp.safety != QuotingSafety::AK47) return;
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
        const Amount bestWidthSize = (sideAPR == SideAPR::Off ? qp.bestWidthSize : 0);
        Amount depth = 0;
        if (!quotes.ask.empty())
          for (const Level &it : levels.asks)
            if (it.price > quotes.ask.price) {
              depth += it.size;
              if (depth <= bestWidthSize) continue;
              const Price bestAsk = it.price - K.gateway->tickPrice;
              if (bestAsk > quotes.ask.price)
                quotes.ask.price = bestAsk;
              break;
            }
        depth = 0;
        if (!quotes.bid.empty())
          for (const Level &it : levels.bids)
            if (it.price < quotes.bid.price) {
              depth += it.size;
              if (depth <= bestWidthSize) continue;
              const Price bestBid = it.price + K.gateway->tickPrice;
              if (bestBid < quotes.bid.price)
                quotes.bid.price = bestBid;
              break;
            }
      };
      void applyTradesPerMinute() {
        const double factor = (quotes.superSpread and (
          qp.superTrades == SOP::Trades or qp.superTrades == SOP::TradesSize
        )) ? qp.sopWidthMultiplier
           : 1;
        if (!quotes.ask.isPong and wallet.safety.sell >= qp.tradesPerMinute * factor)
          quotes.ask.clear(QuoteState::MaxTradesSeconds);
        if (!quotes.bid.isPong and wallet.safety.buy >= qp.tradesPerMinute * factor)
          quotes.bid.clear(QuoteState::MaxTradesSeconds);
      };
      void applyRoundPrice() {
        if (!quotes.bid.empty())
          quotes.bid.price = fmax(
            0,
            K.gateway->decimal.price.round(quotes.bid.price)
          );
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(
            quotes.bid.price + K.gateway->tickPrice,
            K.gateway->decimal.price.round(quotes.ask.price)
          );
      };
      void applyRoundSize() {
        if (!quotes.bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.bid.price)
            : K.gateway->minSize;
          const Amount maxBid = K.gateway->margin == Future::Spot
            ? wallet.quote.total / quotes.bid.price
            : (K.gateway->margin == Future::Inverse
                ? wallet.base.amount * quotes.bid.price
                : wallet.base.amount / quotes.bid.price
            );
          quotes.bid.size = K.gateway->decimal.amount.round(
            fmax(minBid * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              quotes.bid.size,
              K.gateway->decimal.amount.floor(maxBid)
            ))
          );
        }
        if (!quotes.ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.ask.price)
            : K.gateway->minSize;
          const Amount maxAsk = K.gateway->margin == Future::Spot
            ? wallet.base.total
            : (K.gateway->margin == Future::Inverse
                ? (quotes.bid.empty()
                  ? wallet.base.amount * quotes.ask.price
                  : quotes.bid.size)
                : wallet.base.amount / quotes.ask.price
            );
          quotes.ask.size = K.gateway->decimal.amount.round(
            fmax(minAsk * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              quotes.ask.size,
              K.gateway->decimal.amount.floor(maxAsk)
            ))
          );
        }
      };
      void applyDepleted() {
        if (!quotes.bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.bid.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.quote.total / quotes.bid.price
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.bid.price
                  : wallet.base.amount / quotes.bid.price)
              ) < minBid
          ) quotes.bid.clear(QuoteState::DepletedFunds);
        }
        if (!quotes.ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.ask.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.base.total
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.ask.price
                  : wallet.base.amount / quotes.ask.price)
              ) < minAsk
          ) quotes.ask.clear(QuoteState::DepletedFunds);
        }
      };
      void applyWaitingPing() {
        if (qp.safety == QuotingSafety::Off) return;
        if (!quotes.ask.isPong and (
          (quotes.bid.state != QuoteState::DepletedFunds and (qp.pingAt == PingAt::DepletedSide or qp.pingAt == PingAt::DepletedBidSide))
          or qp.pingAt == PingAt::StopPings
          or qp.pingAt == PingAt::BidSide
          or qp.pingAt == PingAt::DepletedAskSide
        )) quotes.ask.clear(QuoteState::WaitingPing);
        if (!quotes.bid.isPong and (
          (quotes.ask.state != QuoteState::DepletedFunds and (qp.pingAt == PingAt::DepletedSide or qp.pingAt == PingAt::DepletedAskSide))
          or qp.pingAt == PingAt::StopPings
          or qp.pingAt == PingAt::AskSide
          or qp.pingAt == PingAt::DepletedBidSide
        )) quotes.bid.clear(QuoteState::WaitingPing);
      };
      void applyEwmaTrendProtection() {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold)
          quotes.ask.clear(QuoteState::UpTrendHeld);
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold)
          quotes.bid.clear(QuoteState::DownTrendHeld);
      };
  };
  static void to_json(json &j, const AntonioCalculon &k) {
    j = {
      {            "bidStatus", k.quotes.bid.state},
      {            "askStatus", k.quotes.ask.state},
      {              "sideAPR", k.sideAPR         },
      {"quotesInMemoryWaiting", k.countWaiting    },
      {"quotesInMemoryWorking", k.countWorking    },
      {"quotesInMemoryZombies", k.zombies.size()  }
    };
  };

  struct Semaphore: public Client::Broadcast<Semaphore>,
                    public Client::Clickable,
                    public Hotkey::Keymap {
    Connectivity greenButton  = Connectivity::Disconnected,
                 greenGateway = Connectivity::Disconnected;
    private_ref:
      const KryptoNinja &K;
    public:
      Semaphore(const KryptoNinja &bot)
        : Broadcast(bot)
        , Clickable(bot)
        , Keymap(bot, {
            {'Q', [&]() { exit(); }},
            {'q', [&]() { exit(); }},
            {'\e', [&]() { toggle(); }}
          })
        , K(bot)
      {};
      void click(const json &j) override {
        if (j.is_object()
          and j.at("agree").is_number()
          and j.at("agree").get<Connectivity>() != K.gateway->adminAgreement
        ) toggle();
      };
      bool paused() const {
        return !(bool)greenButton;
      };
      bool offline() const {
        return !(bool)greenGateway;
      };
      void read_from_gw(const Connectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          switchFlag();
        }
      };
      mMatter about() const override {
        return mMatter::Connectivity;
      };
    private:
      void toggle() {
        K.gateway->adminAgreement = (Connectivity)!(bool)K.gateway->adminAgreement;
        switchFlag();
      };
      void switchFlag() {
        const Connectivity previous = greenButton;
        greenButton = (Connectivity)(
          (bool)greenGateway and (bool)K.gateway->adminAgreement
        );
        if (greenButton != previous)
          K.log("GW " + K.gateway->exchange, "Quoting state changed to",
            string(paused() ? "DIS" : "") + "CONNECTED");
        broadcast();
        K.repaint();
      };
  };
  static void to_json(json &j, const Semaphore &k) {
    j = {
      { "agree", k.greenButton },
      {"online", k.greenGateway}
    };
  };

  struct Product: public Client::Broadcast<Product> {
    private_ref:
      const KryptoNinja &K;
    public:
      Product(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      json to_json() const {
        return {
          {   "exchange", K.gateway->exchange                         },
          {       "base", K.gateway->base                             },
          {      "quote", K.gateway->quote                            },
          {     "symbol", K.gateway->symbol                           },
          {     "margin", K.gateway->margin                           },
          {  "webMarket", K.gateway->webMarket                        },
          {  "webOrders", K.gateway->webOrders                        },
          {  "tickPrice", K.gateway->decimal.price.stream.precision() },
          {   "tickSize", K.gateway->decimal.amount.stream.precision()},
          {  "stepPrice", K.gateway->decimal.price.step               },
          {   "stepSize", K.gateway->decimal.amount.step              },
          {    "minSize", K.gateway->minSize                          },
          {       "inet", K.arg<string>("interface")                  },
          {"environment", K.arg<string>("title")                      },
          { "matryoshka", K.arg<string>("matryoshka")                 },
          {     "source", K_SOURCE " " K_BUILD                        }
        };
      };
      mMatter about() const override {
        return mMatter::ProductAdvertisement;
      };
  };
  static void to_json(json &j, const Product &k) {
    j = k.to_json();
  };

  struct Memory: public Client::Broadcast<Memory> {
    public:
      unsigned int orders_60s = 0;
    private:
      Product product;
    private_ref:
      const KryptoNinja &K;
    public:
      Memory(const KryptoNinja &bot)
        : Broadcast(bot)
        , product(bot)
        , K(bot)
      {};
      void timer_60s() {
        broadcast();
        orders_60s = 0;
      };
      json to_json() const {
        return {
          {  "addr", K.gateway->unlock           },
          {  "freq", orders_60s                  },
          { "theme", K.arg<int>("ignore-moon")
                       + K.arg<int>("ignore-sun")},
          {"memory", K.memSize()                 },
          {"dbsize", K.dbSize()                  }
        };
      };
      mMatter about() const override {
        return mMatter::ApplicationState;
      };
  };
  static void to_json(json &j, const Memory &k) {
    j = k.to_json();
  };

  struct Broker: public Client::Clicked {
             Memory memory;
          Semaphore semaphore;
    AntonioCalculon calculon;
    private_ref:
      const KryptoNinja   &K;
      const QuotingParams &qp;
            Orders        &orders;
    public:
      Broker(const KryptoNinja &bot, const QuotingParams &q, Orders &o, const Buttons &b, const MarketLevels &l, const Wallets &w)
        : Clicked(bot, {
            {&b.submit, [&](const json &j) { placeOrder(j); }},
            {&b.cancel, [&](const json &j) { cancelOrder(orders.find(j)); }},
            {&b.cancelAll, [&]() { cancelOrders(); }}
          })
        , memory(bot)
        , semaphore(bot)
        , calculon(bot, q, l, w)
        , K(bot)
        , qp(q)
        , orders(o)
      {};
      bool ready() {
        if (semaphore.offline()) {
          calculon.offline();
          return false;
        }
        return true;
      };
      void calcQuotes() {
        if (semaphore.paused()) {
          calculon.paused();
          cancelOrders();
        } else {
          calculon.calcQuotes();
          quote2orders(calculon.quotes.ask);
          quote2orders(calculon.quotes.bid);
        }
      };
      void quote2orders(Quote &quote) {
        const vector<Order*> abandoned = abandon(quote);
        const unsigned int replace = K.gateway->askForReplace and !(
          quote.empty() or abandoned.empty()
        );
        for (
          auto it  =  abandoned.end() - replace;
               it --> abandoned.begin();
          cancelOrder(*it)
        );
        if (quote.empty()) return;
        if (replace)
          replaceOrder(quote.price, quote.isPong, abandoned.back());
        else placeOrder({
          quote.side,
          quote.price,
          quote.size,
          Tstamp,
          quote.isPong,
          K.gateway->randId()
        });
        memory.orders_60s++;
      };
      void purge() {
        for (const Order *const it : calculon.purge())
          orders.purge(it);
      };
      void nuke() {
        cancelOrders();
        K.gateway->cancel();
      };
      void quit() {
        unsigned int n = 0;
        for (Order *const it : orders.open()) {
          K.gateway->cancel(it);
          n++;
        }
        if (n)
          K.log("QE", "Canceled " + to_string(n) + " open order" + string(n != 1, 's') + " before quit");
      };
    private:
      vector<Order*> abandon(Quote &quote) {
        vector<Order*> abandoned;
        unsigned int bullets = qp.bullets;
        const bool all = quote.state != QuoteState::Live;
        for (Order *const it : orders.at(quote.side))
          if (all or calculon.abandon(*it, quote, bullets))
            abandoned.push_back(it);
        return abandoned;
      };
      void placeOrder(const Order &raw) {
        K.gateway->place(orders.upsert(raw));
      };
      void replaceOrder(const Price &price, const bool &isPong, Order *const order) {
        if (orders.replace(price, isPong, order))
          K.gateway->replace(order);
      };
      void cancelOrder(Order *const order) {
        if (orders.cancel(order))
          K.gateway->cancel(order);
      };
      void cancelOrders() {
        for (Order *const it : orders.working())
          cancelOrder(it);
      };
  };

  class Engine {
    public:
      QuotingParams qp;
             Orders orders;
            Buttons button;
       MarketLevels levels;
            Wallets wallet;
             Broker broker;
    private_ref:
      const KryptoNinja &K;
    public:
      Engine(const KryptoNinja &bot)
        : qp(bot)
        , orders(bot)
        , button(bot)
        , levels(bot, qp, orders)
        , wallet(bot, qp, orders, button, levels)
        , broker(bot, qp, orders, button, levels, wallet)
        , K(bot)
      {};
    public:
      void read(const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
      };
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      void read(const Levels &rawdata) {
        levels.read_from_gw(rawdata);
        wallet.calcFunds();
        calcQuotes();
      };
      void read(const Order &rawdata) {
        orders.read_from_gw(rawdata);
        wallet.calcFundsAfterOrder();
      };
      void read(const Trade &rawdata) {
        levels.stats.takerTrades.read_from_gw(rawdata);
      };
      void timer_1s(const unsigned int &tick) {
        if (levels.ready()) {
          if (qp.cancelOrdersAuto
            and !(tick % 300)
          ) broker.nuke();
          levels.timer_1s();
          if (!(tick % 60)) {
            levels.timer_60s();
            broker.memory.timer_60s();
          }
          wallet.safety.timer_1s();
          calcQuotes();
        }
      };
      void quit() {
        broker.quit();
      };
    private:
      void calcQuotes() {
        if (broker.ready() and levels.ready() and wallet.ready())
          broker.calcQuotes();
        broker.purge();
      };
  };
}

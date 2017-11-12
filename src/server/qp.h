#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  struct Qp {
    double            widthPing                     = 2.0;
    double            widthPingPercentage           = 0.25;
    double            widthPong                     = 2.0;
    double            widthPongPercentage           = 0.25;
    bool              widthPercentage               = false;
    bool              bestWidth                     = true;
    double            buySize                       = 0.02;
    int               buySizePercentage             = 7;
    bool              buySizeMax                    = false;
    double            sellSize                      = 0.01;
    int               sellSizePercentage            = 7;
    bool              sellSizeMax                   = false;
    mPingAt           pingAt                        = mPingAt::BothSides;
    mPongAt           pongAt                        = mPongAt::ShortPingFair;
    mQuotingMode      mode                          = mQuotingMode::Top;
    mQuotingSafety    safety                        = mQuotingSafety::Boomerang;
    int               bullets                       = 2;
    double            range                         = 0.5;
    double            rangePercentage               = 1.0;
    mFairValueModel   fvModel                       = mFairValueModel::BBO;
    double            targetBasePosition            = 1.0;
    int               targetBasePositionPercentage  = 50;
    double            positionDivergence            = 0.9;
    int               positionDivergencePercentage  = 21;
    bool              percentageValues              = false;
    mAutoPositionMode autoPositionMode              = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing = mAPR::Off;
    mSOP              superTrades                   = mSOP::Off;
    double            tradesPerMinute               = 0.9;
    int               tradeRateSeconds              = 3;
    bool              quotingEwmaProtection         = true;
    int               quotingEwmaProtectionPeriods  = 200;
    bool              quotingEwmaSMUProtection      = false;
    double            quotingEwmaSMUThreshold       = 2.0;
    int               quotingEwmaSMPeriods          = 12;
    int               quotingEwmaSUPeriods          = 3;
    mSTDEV            quotingStdevProtection        = mSTDEV::Off;
    bool              quotingStdevBollingerBands    = false;
    double            quotingStdevProtectionFactor  = 1.0;
    int               quotingStdevProtectionPeriods = 1200;
    double            ewmaSensiblityPercentage      = 0.5;
    int               longEwmaPeriods               = 200;
    int               mediumEwmaPeriods             = 100;
    int               shortEwmaPeriods              = 50;
    double            aprMultiplier                 = 2;
    double            sopWidthMultiplier            = 2;
    double            sopSizeMultiplier             = 2;
    double            sopTradesMultiplier           = 2;
    int               delayAPI                      = 0;
    bool              cancelOrdersAuto              = false;
    double            cleanPongsAuto                = 0.0;
    double            profitHourInterval            = 0.5;
    bool              audio                         = false;
    int               delayUI                       = 7;
  } qp;
  void to_json(json& j, const Qp& k) {
    j = {
      {"widthPing", k.widthPing},
      {"widthPingPercentage", k.widthPingPercentage},
      {"widthPong", k.widthPong},
      {"widthPongPercentage", k.widthPongPercentage},
      {"widthPercentage", k.widthPercentage},
      {"bestWidth", k.bestWidth},
      {"buySize", k.buySize},
      {"buySizePercentage", k.buySizePercentage},
      {"buySizeMax", k.buySizeMax},
      {"sellSize", k.sellSize},
      {"sellSizePercentage", k.sellSizePercentage},
      {"sellSizeMax", k.sellSizeMax},
      {"pingAt", (int)k.pingAt},
      {"pongAt", (int)k.pongAt},
      {"mode", (int)k.mode},
      {"safety", (int)k.safety},
      {"bullets", k.bullets},
      {"range", k.range},
      {"rangePercentage", k.rangePercentage},
      {"fvModel", (int)k.fvModel},
      {"targetBasePosition", k.targetBasePosition},
      {"targetBasePositionPercentage", k.targetBasePositionPercentage},
      {"positionDivergence", k.positionDivergence},
      {"positionDivergencePercentage", k.positionDivergencePercentage},
      {"percentageValues", k.percentageValues},
      {"autoPositionMode", (int)k.autoPositionMode},
      {"aggressivePositionRebalancing", (int)k.aggressivePositionRebalancing},
      {"superTrades", (int)k.superTrades},
      {"tradesPerMinute", k.tradesPerMinute},
      {"tradeRateSeconds", k.tradeRateSeconds},
      {"quotingEwmaProtection", k.quotingEwmaProtection},
      {"quotingEwmaProtectionPeriods", k.quotingEwmaProtectionPeriods},
      {"quotingEwmaSMUProtection", k.quotingEwmaSMUProtection},
      {"quotingEwmaSMUThreshold", k.quotingEwmaSMUThreshold},
      {"quotingEwmaSMPeriods", k.quotingEwmaSMPeriods},
      {"quotingEwmaSUPeriods", k.quotingEwmaSUPeriods},
      {"quotingStdevProtection", (int)k.quotingStdevProtection},
      {"quotingStdevBollingerBands", k.quotingStdevBollingerBands},
      {"quotingStdevProtectionFactor", k.quotingStdevProtectionFactor},
      {"quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods},
      {"ewmaSensiblityPercentage", k.ewmaSensiblityPercentage},
      {"longEwmaPeriods", k.longEwmaPeriods},
      {"mediumEwmaPeriods", k.mediumEwmaPeriods},
      {"shortEwmaPeriods", k.shortEwmaPeriods},
      {"aprMultiplier", k.aprMultiplier},
      {"sopWidthMultiplier", k.sopWidthMultiplier},
      {"sopSizeMultiplier", k.sopSizeMultiplier},
      {"sopTradesMultiplier", k.sopTradesMultiplier},
      {"delayAPI", k.delayAPI},
      {"cancelOrdersAuto", k.cancelOrdersAuto},
      {"cleanPongsAuto", k.cleanPongsAuto},
      {"profitHourInterval", k.profitHourInterval},
      {"audio", k.audio},
      {"delayUI", k.delayUI}
    };
  };
  void from_json(const json& j, Qp& k) {
    if (j.end() != j.find("widthPing")) k.widthPing = j.at("widthPing").get<double>();
    if (j.end() != j.find("widthPingPercentage")) k.widthPingPercentage = j.at("widthPingPercentage").get<double>();
    if (j.end() != j.find("widthPong")) k.widthPong = j.at("widthPong").get<double>();
    if (j.end() != j.find("widthPongPercentage")) k.widthPongPercentage = j.at("widthPongPercentage").get<double>();
    if (j.end() != j.find("widthPercentage")) k.widthPercentage = j.at("widthPercentage").get<bool>();
    if (j.end() != j.find("bestWidth")) k.bestWidth = j.at("bestWidth").get<bool>();
    if (j.end() != j.find("buySize")) k.buySize = j.at("buySize").get<double>();
    if (j.end() != j.find("buySizePercentage")) k.buySizePercentage = j.at("buySizePercentage").get<int>();
    if (j.end() != j.find("buySizeMax")) k.buySizeMax = j.at("buySizeMax").get<bool>();
    if (j.end() != j.find("sellSize")) k.sellSize = j.at("sellSize").get<double>();
    if (j.end() != j.find("sellSizePercentage")) k.sellSizePercentage = j.at("sellSizePercentage").get<int>();
    if (j.end() != j.find("sellSizeMax")) k.sellSizeMax = j.at("sellSizeMax").get<bool>();
    if (j.end() != j.find("pingAt")) k.pingAt = (mPingAt)j.at("pingAt").get<int>();
    if (j.end() != j.find("pongAt")) k.pongAt = (mPongAt)j.at("pongAt").get<int>();
    if (j.end() != j.find("mode")) k.mode = (mQuotingMode)j.at("mode").get<int>();
    if (j.end() != j.find("safety")) k.safety = (mQuotingSafety)j.at("safety").get<int>();
    if (j.end() != j.find("bullets")) k.bullets = j.at("bullets").get<int>();
    if (j.end() != j.find("range")) k.range = j.at("range").get<double>();
    if (j.end() != j.find("rangePercentage")) k.rangePercentage = j.at("rangePercentage").get<double>();
    if (j.end() != j.find("fvModel")) k.fvModel = (mFairValueModel)j.at("fvModel").get<int>();
    if (j.end() != j.find("targetBasePosition")) k.targetBasePosition = j.at("targetBasePosition").get<double>();
    if (j.end() != j.find("targetBasePositionPercentage")) k.targetBasePositionPercentage = j.at("targetBasePositionPercentage").get<int>();
    if (j.end() != j.find("positionDivergence")) k.positionDivergence = j.at("positionDivergence").get<double>();
    if (j.end() != j.find("positionDivergencePercentage")) k.positionDivergencePercentage = j.at("positionDivergencePercentage").get<int>();
    if (j.end() != j.find("percentageValues")) k.percentageValues = j.at("percentageValues").get<bool>();
    if (j.end() != j.find("autoPositionMode")) k.autoPositionMode = (mAutoPositionMode)j.at("autoPositionMode").get<int>();
    if (j.end() != j.find("aggressivePositionRebalancing")) k.aggressivePositionRebalancing = (mAPR)j.at("aggressivePositionRebalancing").get<int>();
    if (j.end() != j.find("superTrades")) k.superTrades = (mSOP)j.at("superTrades").get<int>();
    if (j.end() != j.find("tradesPerMinute")) k.tradesPerMinute = j.at("tradesPerMinute").get<double>();
    if (j.end() != j.find("tradeRateSeconds")) k.tradeRateSeconds = j.at("tradeRateSeconds").get<int>();
    if (j.end() != j.find("quotingEwmaProtection")) k.quotingEwmaProtection = j.at("quotingEwmaProtection").get<bool>();
    if (j.end() != j.find("quotingEwmaProtectionPeriods")) k.quotingEwmaProtectionPeriods = j.at("quotingEwmaProtectionPeriods").get<int>();
    if (j.end() != j.find("quotingEwmaSMUProtection")) k.quotingEwmaSMUProtection = j.at("quotingEwmaSMUProtection").get<bool>();
    if (j.end() != j.find("quotingEwmaSMUThreshold")) k.quotingEwmaSMUThreshold = j.at("quotingEwmaSMUThreshold").get<double>();
    if (j.end() != j.find("quotingEwmaSMPeriods")) k.quotingEwmaSMPeriods = j.at("quotingEwmaSMPeriods").get<int>();
    if (j.end() != j.find("quotingEwmaSUPeriods")) k.quotingEwmaSUPeriods = j.at("quotingEwmaSUPeriods").get<int>();
    if (j.end() != j.find("quotingStdevProtection")) k.quotingStdevProtection = (mSTDEV)j.at("quotingStdevProtection").get<int>();
    if (j.end() != j.find("quotingStdevBollingerBands")) k.quotingStdevBollingerBands = j.at("quotingStdevBollingerBands").get<bool>();
    if (j.end() != j.find("quotingStdevProtectionFactor")) k.quotingStdevProtectionFactor = j.at("quotingStdevProtectionFactor").get<double>();
    if (j.end() != j.find("quotingStdevProtectionPeriods")) k.quotingStdevProtectionPeriods = j.at("quotingStdevProtectionPeriods").get<int>();
    if (j.end() != j.find("ewmaSensiblityPercentage")) k.ewmaSensiblityPercentage = j.at("ewmaSensiblityPercentage").get<double>();
    if (j.end() != j.find("longEwmaPeriods")) k.longEwmaPeriods = j.at("longEwmaPeriods").get<int>();
    if (j.end() != j.find("mediumEwmaPeriods")) k.mediumEwmaPeriods = j.at("mediumEwmaPeriods").get<int>();
    if (j.end() != j.find("shortEwmaPeriods")) k.shortEwmaPeriods = j.at("shortEwmaPeriods").get<int>();
    if (j.end() != j.find("aprMultiplier")) k.aprMultiplier = j.at("aprMultiplier").get<double>();
    if (j.end() != j.find("sopWidthMultiplier")) k.sopWidthMultiplier = j.at("sopWidthMultiplier").get<double>();
    if (j.end() != j.find("sopSizeMultiplier")) k.sopSizeMultiplier = j.at("sopSizeMultiplier").get<double>();
    if (j.end() != j.find("sopTradesMultiplier")) k.sopTradesMultiplier = j.at("sopTradesMultiplier").get<double>();
    if (j.end() != j.find("delayAPI")) k.delayAPI = j.at("delayAPI").get<int>();
    if (j.end() != j.find("cancelOrdersAuto")) k.cancelOrdersAuto = j.at("cancelOrdersAuto").get<bool>();
    if (j.end() != j.find("cleanPongsAuto")) k.cleanPongsAuto = j.at("cleanPongsAuto").get<double>();
    if (j.end() != j.find("profitHourInterval")) k.profitHourInterval = j.at("profitHourInterval").get<double>();
    if (j.end() != j.find("audio")) k.audio = j.at("audio").get<bool>();
    if (j.end() != j.find("delayUI")) k.delayUI = j.at("delayUI").get<int>();
    if ((int)k.mode > 6) k.mode = mQuotingMode::Top; // remove after everybody have the new mode/safety in their databases (2018)
    if (k.mode == mQuotingMode::Depth) k.widthPercentage = false;
  };
  class QP: public Klass {
    protected:
      void load() {
        json k = ((DB*)evDB)->load(uiTXT::QuotingParametersChange);
        if (k.size()) {
          qp = k.at(0);
          FN::log("DB", "loaded Quoting Parameters OK");
        } else FN::logWar("QP", "using default values for Quoting Parameters");
      };
      void waitUser() {
        ((UI*)evUI)->evSnap(uiTXT::QuotingParametersChange, &onSnap);
        ((UI*)evUI)->evHand(uiTXT::QuotingParametersChange, &onHand);
      };
      void run() {
        ((UI*)evUI)->evDelay(qp.delayUI);
      };
    public:
      bool matchPings() {
        return qp.safety == mQuotingSafety::Boomerang
            or qp.safety == mQuotingSafety::AK47;
      };
    private:
      function<json()> onSnap = []() {
        return (json){ qp };
      };
      function<void(json)> onHand = [&](json k) {
        if (k.value("buySize", 0.0) > 0
          and k.value("sellSize", 0.0) > 0
          and k.value("buySizePercentage", 0.0) > 0
          and k.value("sellSizePercentage", 0.0) > 0
          and k.value("widthPing", 0.0) > 0
          and k.value("widthPong", 0.0) > 0
          and k.value("widthPingPercentage", 0.0) > 0
          and k.value("widthPongPercentage", 0.0) > 0
        ) {
          qp = k;
          ((DB*)evDB)->insert(uiTXT::QuotingParametersChange, qp);
          ((EV*)evEV)->uiQuotingParameters();
          ((UI*)evUI)->evDelay(qp.delayUI);
        }
        ((UI*)evUI)->evSend(uiTXT::QuotingParametersChange, qp, false);
      };
  };
}

#endif

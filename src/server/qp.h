#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  static struct Qp {
    double            widthPing                     = decimal_cast<1>("2.0").getAsDouble();
    double            widthPingPercentage           = decimal_cast<2>("0.25").getAsDouble();
    double            widthPong                     = decimal_cast<1>("2.0").getAsDouble();
    double            widthPongPercentage           = decimal_cast<2>("0.25").getAsDouble();
    bool              widthPercentage               = false;
    bool              bestWidth                     = true;
    double            buySize                       = decimal_cast<2>("0.02").getAsDouble();
    int               buySizePercentage             = 7;
    bool              buySizeMax                    = false;
    double            sellSize                      = decimal_cast<2>("0.01").getAsDouble();
    int               sellSizePercentage            = 7;
    bool              sellSizeMax                   = false;
    mPingAt           pingAt                        = mPingAt::BothSides;
    mPongAt           pongAt                        = mPongAt::ShortPingFair;
    mQuotingMode      mode                          = mQuotingMode::AK47;
    int               bullets                       = 2;
    double            range                         = decimal_cast<1>("0.5").getAsDouble();
    mFairValueModel   fvModel                       = mFairValueModel::BBO;
    double            targetBasePosition            = decimal_cast<1>("1.0").getAsDouble();
    int               targetBasePositionPercentage  = 50;
    double            positionDivergence            = decimal_cast<1>("0.9").getAsDouble();
    int               positionDivergencePercentage  = 21;
    bool              percentageValues              = false;
    mAutoPositionMode autoPositionMode              = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing = mAPR::Off;
    mSOP              superTrades                   = mSOP::Off;
    double            tradesPerMinute               = decimal_cast<1>("0.9").getAsDouble();
    int               tradeRateSeconds              = 69;
    bool              quotingEwmaProtection         = true;
    int               quotingEwmaProtectionPeriods  = 200;
    mSTDEV            quotingStdevProtection        = mSTDEV::Off;
    bool              quotingStdevBollingerBands    = false;
    double            quotingStdevProtectionFactor  = decimal_cast<1>("1.0").getAsDouble();
    int               quotingStdevProtectionPeriods = 1200;
    double            ewmaSensiblityPercentage      = decimal_cast<1>("0.5").getAsDouble();
    int               longEwmaPeriods               = 200;
    int               mediumEwmaPeriods             = 100;
    int               shortEwmaPeriods              = 50;
    int               aprMultiplier                 = 2;
    int               sopWidthMultiplier            = 2;
    int               delayAPI                      = 0;
    bool              cancelOrdersAuto              = false;
    double            cleanPongsAuto                = decimal_cast<1>("0.0").getAsDouble();
    double            profitHourInterval            = decimal_cast<1>("0.5").getAsDouble();
    bool              audio                         = false;
    int               delayUI                       = 7;
  } qp;
  static void to_json(json& j, const Qp& k) {
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
      {"bullets", k.bullets},
      {"range", k.range},
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
      {"delayAPI", k.delayAPI},
      {"cancelOrdersAuto", k.cancelOrdersAuto},
      {"cleanPongsAuto", k.cleanPongsAuto},
      {"profitHourInterval", k.profitHourInterval},
      {"audio", k.audio},
      {"delayUI", k.delayUI}
    };
  };
  static void from_json(const json& j, Qp& k) {
    if (!j.at("widthPing").is_null()) k.widthPing = j.at("widthPing").get<double>();
    if (!j.at("widthPingPercentage").is_null()) k.widthPingPercentage = j.at("widthPingPercentage").get<double>();
    if (!j.at("widthPong").is_null()) k.widthPong = j.at("widthPong").get<double>();
    if (!j.at("widthPongPercentage").is_null()) k.widthPongPercentage = j.at("widthPongPercentage").get<double>();
    if (!j.at("widthPercentage").is_null()) k.widthPercentage = j.at("widthPercentage").get<bool>();
    if (!j.at("bestWidth").is_null()) k.bestWidth = j.at("bestWidth").get<bool>();
    if (!j.at("buySize").is_null()) k.buySize = j.at("buySize").get<double>();
    if (!j.at("buySizePercentage").is_null()) k.buySizePercentage = j.at("buySizePercentage").get<int>();
    if (!j.at("buySizeMax").is_null()) k.buySizeMax = j.at("buySizeMax").get<bool>();
    if (!j.at("sellSize").is_null()) k.sellSize = j.at("sellSize").get<double>();
    if (!j.at("sellSizePercentage").is_null()) k.sellSizePercentage = j.at("sellSizePercentage").get<int>();
    if (!j.at("sellSizeMax").is_null()) k.sellSizeMax = j.at("sellSizeMax").get<bool>();
    if (!j.at("pingAt").is_null()) k.pingAt = (mPingAt)j.at("pingAt").get<int>();
    if (!j.at("pongAt").is_null()) k.pongAt = (mPongAt)j.at("pongAt").get<int>();
    if (!j.at("mode").is_null()) k.mode = (mQuotingMode)j.at("mode").get<int>();
    if (!j.at("bullets").is_null()) k.bullets = j.at("bullets").get<int>();
    if (!j.at("range").is_null()) k.range = j.at("range").get<double>();
    if (!j.at("fvModel").is_null()) k.fvModel = (mFairValueModel)j.at("fvModel").get<int>();
    if (!j.at("targetBasePosition").is_null()) k.targetBasePosition = j.at("targetBasePosition").get<double>();
    if (!j.at("targetBasePositionPercentage").is_null()) k.targetBasePositionPercentage = j.at("targetBasePositionPercentage").get<int>();
    if (!j.at("positionDivergence").is_null()) k.positionDivergence = j.at("positionDivergence").get<double>();
    if (!j.at("positionDivergencePercentage").is_null()) k.positionDivergencePercentage = j.at("positionDivergencePercentage").get<int>();
    if (!j.at("percentageValues").is_null()) k.percentageValues = j.at("percentageValues").get<bool>();
    if (!j.at("autoPositionMode").is_null()) k.autoPositionMode = (mAutoPositionMode)j.at("autoPositionMode").get<int>();
    if (!j.at("aggressivePositionRebalancing").is_null()) k.aggressivePositionRebalancing = (mAPR)j.at("aggressivePositionRebalancing").get<int>();
    if (!j.at("superTrades").is_null()) k.superTrades = (mSOP)j.at("superTrades").get<int>();
    if (!j.at("tradesPerMinute").is_null()) k.tradesPerMinute = j.at("tradesPerMinute").get<double>();
    if (!j.at("tradeRateSeconds").is_null()) k.tradeRateSeconds = j.at("tradeRateSeconds").get<int>();
    if (!j.at("quotingEwmaProtection").is_null()) k.quotingEwmaProtection = j.at("quotingEwmaProtection").get<bool>();
    if (!j.at("quotingEwmaProtectionPeriods").is_null()) k.quotingEwmaProtectionPeriods = j.at("quotingEwmaProtectionPeriods").get<int>();
    if (!j.at("quotingStdevProtection").is_null()) k.quotingStdevProtection = (mSTDEV)j.at("quotingStdevProtection").get<int>();
    if (!j.at("quotingStdevBollingerBands").is_null()) k.quotingStdevBollingerBands = j.at("quotingStdevBollingerBands").get<bool>();
    if (!j.at("quotingStdevProtectionFactor").is_null()) k.quotingStdevProtectionFactor = j.at("quotingStdevProtectionFactor").get<double>();
    if (!j.at("quotingStdevProtectionPeriods").is_null()) k.quotingStdevProtectionPeriods = j.at("quotingStdevProtectionPeriods").get<int>();
    if (!j.at("ewmaSensiblityPercentage").is_null()) k.ewmaSensiblityPercentage = j.at("ewmaSensiblityPercentage").get<double>();
    if (!j.at("longEwmaPeriods").is_null()) k.longEwmaPeriods = j.at("longEwmaPeriods").get<int>();
    if (!j.at("mediumEwmaPeriods").is_null()) k.mediumEwmaPeriods = j.at("mediumEwmaPeriods").get<int>();
    if (!j.at("shortEwmaPeriods").is_null()) k.shortEwmaPeriods = j.at("shortEwmaPeriods").get<int>();
    if (!j.at("aprMultiplier").is_null()) k.aprMultiplier = j.at("aprMultiplier").get<int>();
    if (!j.at("sopWidthMultiplier").is_null()) k.sopWidthMultiplier = j.at("sopWidthMultiplier").get<int>();
    if (!j.at("delayAPI").is_null()) k.delayAPI = j.at("delayAPI").get<int>();
    if (!j.at("cancelOrdersAuto").is_null()) k.cancelOrdersAuto = j.at("cancelOrdersAuto").get<bool>();
    if (!j.at("cleanPongsAuto").is_null()) k.cleanPongsAuto = j.at("cleanPongsAuto").get<double>();
    if (!j.at("profitHourInterval").is_null()) k.profitHourInterval = j.at("profitHourInterval").get<double>();
    if (!j.at("audio").is_null()) k.audio = j.at("audio").get<bool>();
    if (!j.at("delayUI").is_null()) k.delayUI = j.at("delayUI").get<int>();
    if (k.mode == mQuotingMode::Depth) k.widthPercentage = false;
  };
  class QP {
    public:
      static void main() {
        load();
        UI::uiSnap(uiTXT::QuotingParametersChange, &onSnap);
        UI::uiHand(uiTXT::QuotingParametersChange, &onHand);
      }
      static bool matchPings() {
        return qp.mode == mQuotingMode::Boomerang
            or qp.mode == mQuotingMode::HamelinRat
            or qp.mode == mQuotingMode::AK47;
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::QuotingParametersChange);
        if (k.size())
          qp = k.at(0);
        UI::delay(qp.delayUI);
        FN::log("DB", string("loaded Quoting Parameters ") + (k.size() ? "OK" : "OR reading defaults instead"));
      };
      static json onSnap() {
        return { qp };
      };
      static void onHand(json k) {
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
          DB::insert(uiTXT::QuotingParametersChange, qp);
          ev_uiQuotingParameters();
          UI::delay(qp.delayUI);
        }
        UI::uiSend(uiTXT::QuotingParametersChange, qp);
      };
  };
}

#endif

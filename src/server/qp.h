#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  static json qp;
  static json defQP {
    {  "widthPing",                     2                                      },
    {  "widthPingPercentage",           decimal_cast<2>("0.25").getAsDouble()  },
    {  "widthPong",                     2                                      },
    {  "widthPongPercentage",           decimal_cast<2>("0.25").getAsDouble()  },
    {  "widthPercentage",               false                                  },
    {  "bestWidth",                     true                                   },
    {  "buySize",                       decimal_cast<2>("0.02").getAsDouble()  },
    {  "buySizePercentage",             7                                      },
    {  "buySizeMax",                    false                                  },
    {  "sellSize",                      decimal_cast<2>("0.01").getAsDouble()  },
    {  "sellSizePercentage",            7                                      },
    {  "sellSizeMax",                   false                                  },
    {  "pingAt",                        (int)mPingAt::BothSides                },
    {  "pongAt",                        (int)mPongAt::ShortPingFair            },
    {  "mode",                          (int)mQuotingMode::AK47                },
    {  "bullets",                       2                                      },
    {  "range",                         decimal_cast<1>("0.5").getAsDouble()   },
    {  "fvModel",                       (int)mFairValueModel::BBO              },
    {  "targetBasePosition",            1                                      },
    {  "targetBasePositionPercentage",  50                                     },
    {  "positionDivergence",            decimal_cast<1>("0.9").getAsDouble()   },
    {  "positionDivergencePercentage",  21                                     },
    {  "percentageValues",              false                                  },
    {  "autoPositionMode",              (int)mAutoPositionMode::EWMA_LS        },
    {  "aggressivePositionRebalancing", (int)mAPR::Off                         },
    {  "superTrades",                   (int)mSOP::Off                         },
    {  "tradesPerMinute",               decimal_cast<1>("0.9").getAsDouble()   },
    {  "tradeRateSeconds",              69                                     },
    {  "quotingEwmaProtection",         true                                   },
    {  "quotingEwmaProtectionPeriods",  200                                    },
    {  "quotingStdevProtection",        (int)mSTDEV::Off                       },
    {  "quotingStdevBollingerBands",    false                                  },
    {  "quotingStdevProtectionFactor",  decimal_cast<1>("1.0").getAsDouble()   },
    {  "quotingStdevProtectionPeriods", 1200                                   },
    {  "ewmaSensiblityPercentage",      decimal_cast<1>("0.5").getAsDouble()   },
    {  "longEwmaPeriods",               200                                    },
    {  "mediumEwmaPeriods",             100                                    },
    {  "shortEwmaPeriods",              50                                     },
    {  "aprMultiplier",                 2                                      },
    {  "sopWidthMultiplier",            2                                      },
    {  "delayAPI",                      0                                      },
    {  "cancelOrdersAuto",              false                                  },
    {  "cleanPongsAuto",                0                                      },
    {  "profitHourInterval",            decimal_cast<1>("0.5").getAsDouble()   },
    {  "audio",                         false                                  },
    {  "delayUI",                       7                                      }
  };
  static const vector<string> boolQP = {
    "widthPercentage", "bestWidth", "sellSizeMax", "buySizeMax", "percentageValues",
    "quotingEwmaProtection", "quotingStdevBollingerBands", "cancelOrdersAuto", "audio"
  };
  class QP {
    public:
      static void main() {
        load();
        UI::uiSnap(uiTXT::QuotingParametersChange, &onSnap);
        UI::uiHand(uiTXT::QuotingParametersChange, &onHand);
      }
      static bool matchPings() {
        mQuotingMode k = (mQuotingMode)getInt("mode");
        return k == mQuotingMode::Boomerang
            or k == mQuotingMode::HamelinRat
            or k == mQuotingMode::AK47;
      };
      static bool getBool(string k) {
        if (!qp[k].is_boolean()) {
          cout << FN::uiT() << "QP" << RRED << " Warrrrning:" << BRED << " Warrrrning: " << k << " is not boolean, get a false instead." << endl;
          return false;
        }
        return qp[k].get<bool>();
      };
      static int getInt(string k) {
        if (!qp[k].is_number()) {
          cout << FN::uiT() << "QP" << RRED << " Warrrrning:" << BRED << " Warrrrning: " << k << " is not numeric, get a 0 instead." << endl;
          return 0;
        }
        return qp[k].get<int>();
      };
      static double getDouble(string k) {
        if (!qp[k].is_number()) {
          cout << FN::uiT() << "QP" << RRED << " Warrrrning:" << BRED << " Warrrrning: " << k << " is not numeric, get a 0 instead." << endl;
          return 0;
        }
        return qp[k].get<double>();
      };
    private:
      static void load() {
        qp = defQP;
        json qp_ = DB::load(uiTXT::QuotingParametersChange);
        if (qp_.size()) {
          qp_ = qp_.at(0);
          for (json::iterator it = qp_.begin(); it != qp_.end(); ++it)
            qp[it.key()] = it.value();
        }
        clean();
        UI::delay(getDouble("delayUI"));
        cout << FN::uiT() << "DB" << RWHITE << " loaded Quoting Parameters " << (qp_.size() ? "OK" : "OR reading defaults instead") << "." << endl;
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
          if ((mQuotingMode)k.value("mode", 0) == mQuotingMode::Depth)
            k["widthPercentage"] = false;
          qp = k;
          clean();
          DB::insert(uiTXT::QuotingParametersChange, k);
          ev_uiQuotingParameters();
          UI::delay(getDouble("delayUI"));
        }
        UI::uiSend(uiTXT::QuotingParametersChange, k);
      };
      static void clean() {
        for (vector<string>::const_iterator it = boolQP.begin(); it != boolQP.end(); ++it)
          if (qp[*it].is_number()) qp[*it] = qp[*it].get<int>() != 0;
      };
  };
}

#endif

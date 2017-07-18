#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  struct Qp {
    int               widthPing                     = 2;
    double            widthPingPercentage           = decimal_cast<2>("0.25").getAsDouble();
    int               widthPong                     = 2;
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
    int               targetBasePosition            = 1;
    int               getBasePositionPercentage     = 50;
    double            positionDivergence            = decimal_cast<1>("0.9").getAsDouble();
    int               positionDivergencePercentage  = 21;
    bool              percentageValues              = false;
    mAutoPositionMode autoPositionMode              = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing = mAPR::Off;
    mSOP              superTrades                   = mSOP::Off;
    double            tradesPerMinute               = decimal_cast<1>("0.9").getAsDouble();
    int               tradeRateSeconds              = 69;
    bool              quotingEwmaProtection         = true;
    int               quotingEwmaProtectionPeridos  = 200;
    mSTDEV            quotingStdevProtection        = mSTDEV::Off;
    bool              quotingStdevBollingerBands    = false;
    double            quotingStdevProtectionFactor  = decimal_cast<1>("1.0").getAsDouble();
    int               quotingStdevProtectionPeriods = 1200;
    double            ewmaSensiblityPercentage      = decimal_cast<1>("0.5").getAsDouble();
    int               longEwmaPeridos               = 200;
    int               mediumEwmaPeridos             = 100;
    int               shortEwmaPeridos              = 50;
    int               aprMultiplier                 = 2;
    int               sopWidthMultiplier            = 2;
    int               delayAPI                      = 0;
    bool              cancelOrdersAuto              = false;
    int               cleanPongsAuto                = 0;
    double            profitHourInterval            = decimal_cast<1>("0.5").getAsDouble();
    bool              audio                         = false;
    int               delayUI                       = 7;
  } qp;
  class QP {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        Qp *qp = new Qp;
        Local<Object> qpRepo_ = Object::New(isolate);
        qpRepo_->Set(FN::v8S("widthPing"), Number::New(isolate, qp->widthPing));
        qpRepo_->Set(FN::v8S("widthPingPercentage"), Number::New(isolate, qp->widthPingPercentage));
        qpRepo_->Set(FN::v8S("widthPong"), Number::New(isolate, qp->widthPong));
        qpRepo_->Set(FN::v8S("widthPongPercentage"), Number::New(isolate, qp->widthPongPercentage));
        qpRepo_->Set(FN::v8S("widthPercentage"), Boolean::New(isolate, qp->widthPercentage));
        qpRepo_->Set(FN::v8S("bestWidth"), Boolean::New(isolate, qp->bestWidth));
        qpRepo_->Set(FN::v8S("buySize"), Number::New(isolate, qp->buySize));
        qpRepo_->Set(FN::v8S("buySizePercentage"), Number::New(isolate, qp->buySizePercentage));
        qpRepo_->Set(FN::v8S("buySizeMax"), Boolean::New(isolate, qp->buySizeMax));
        qpRepo_->Set(FN::v8S("sellSize"), Number::New(isolate, qp->sellSize));
        qpRepo_->Set(FN::v8S("sellSizePercentage"), Number::New(isolate, qp->sellSizePercentage));
        qpRepo_->Set(FN::v8S("sellSizeMax"), Boolean::New(isolate, qp->sellSizeMax));
        qpRepo_->Set(FN::v8S("pingAt"), Number::New(isolate, (int)qp->pingAt));
        qpRepo_->Set(FN::v8S("pongAt"), Number::New(isolate, (int)qp->pongAt));
        qpRepo_->Set(FN::v8S("mode"), Number::New(isolate, (int)qp->mode));
        qpRepo_->Set(FN::v8S("bullets"), Number::New(isolate, qp->bullets));
        qpRepo_->Set(FN::v8S("range"), Number::New(isolate, qp->range));
        qpRepo_->Set(FN::v8S("fvModel"), Number::New(isolate, (int)qp->fvModel));
        qpRepo_->Set(FN::v8S("targetBasePosition"), Number::New(isolate, qp->targetBasePosition));
        qpRepo_->Set(FN::v8S("getBasePositionPercentage"), Number::New(isolate, qp->getBasePositionPercentage));
        qpRepo_->Set(FN::v8S("positionDivergence"), Number::New(isolate, qp->positionDivergence));
        qpRepo_->Set(FN::v8S("positionDivergencePercentage"), Number::New(isolate, qp->positionDivergencePercentage));
        qpRepo_->Set(FN::v8S("percentageValues"), Boolean::New(isolate, qp->percentageValues));
        qpRepo_->Set(FN::v8S("autoPositionMode"), Number::New(isolate, (int)qp->autoPositionMode));
        qpRepo_->Set(FN::v8S("aggressivePositionRebalancing"), Number::New(isolate, (int)qp->aggressivePositionRebalancing));
        qpRepo_->Set(FN::v8S("superTrades"), Number::New(isolate, (int)qp->superTrades));
        qpRepo_->Set(FN::v8S("tradesPerMinute"), Number::New(isolate, qp->tradesPerMinute));
        qpRepo_->Set(FN::v8S("tradeRateSeconds"), Number::New(isolate, qp->tradeRateSeconds));
        qpRepo_->Set(FN::v8S("quotingEwmaProtection"), Boolean::New(isolate, qp->quotingEwmaProtection));
        qpRepo_->Set(FN::v8S("quotingEwmaProtectionPeridos"), Number::New(isolate, qp->quotingEwmaProtectionPeridos));
        qpRepo_->Set(FN::v8S("quotingStdevProtection"), Number::New(isolate, (int)qp->quotingStdevProtection));
        qpRepo_->Set(FN::v8S("quotingStdevBollingerBands"), Boolean::New(isolate, qp->quotingStdevBollingerBands));
        qpRepo_->Set(FN::v8S("quotingStdevProtectionFactor"), Number::New(isolate, qp->quotingStdevProtectionFactor));
        qpRepo_->Set(FN::v8S("quotingStdevProtectionPeriods"), Number::New(isolate, qp->quotingStdevProtectionPeriods));
        qpRepo_->Set(FN::v8S("ewmaSensiblityPercentage"), Number::New(isolate, qp->ewmaSensiblityPercentage));
        qpRepo_->Set(FN::v8S("longEwmaPeridos"), Number::New(isolate, qp->longEwmaPeridos));
        qpRepo_->Set(FN::v8S("mediumEwmaPeridos"), Number::New(isolate, qp->mediumEwmaPeridos));
        qpRepo_->Set(FN::v8S("shortEwmaPeridos"), Number::New(isolate, qp->shortEwmaPeridos));
        qpRepo_->Set(FN::v8S("aprMultiplier"), Number::New(isolate, qp->aprMultiplier));
        qpRepo_->Set(FN::v8S("sopWidthMultiplier"), Number::New(isolate, qp->sopWidthMultiplier));
        qpRepo_->Set(FN::v8S("delayAPI"), Number::New(isolate, qp->delayAPI));
        qpRepo_->Set(FN::v8S("cancelOrdersAuto"), Boolean::New(isolate, qp->cancelOrdersAuto));
        qpRepo_->Set(FN::v8S("cleanPongsAuto"), Number::New(isolate, qp->cleanPongsAuto));
        qpRepo_->Set(FN::v8S("profitHourInterval"), Number::New(isolate, qp->profitHourInterval));
        qpRepo_->Set(FN::v8S("audio"), Boolean::New(isolate, qp->audio));
        qpRepo_->Set(FN::v8S("delayUI"), Number::New(isolate, qp->delayUI));
        MaybeLocal<Array> maybe_props;
        Local<Array> props;
        maybe_props = qpRepo_->GetOwnPropertyNames(Context::New(isolate));
        if (!maybe_props.IsEmpty()) {
          props = maybe_props.ToLocalChecked();
          for(uint32_t i=0; i < props->Length(); i++) {
            string k = CF::cfString(FN::S8v(props->Get(i)->ToString()), false);
            if (k != "") qpRepo_->Set(props->Get(i)->ToString(), Number::New(isolate, stod(k)));
          }
        }
        Local<Value> qpa = DB::load(isolate, string(1, (char)uiTXT::QuotingParametersChange));
        if (FN::S8v(qpa->ToString()).length()) {
          Local<Object> qp_ = qpa->ToObject()->Get(0)->ToObject();
          maybe_props = qp_->GetOwnPropertyNames(Context::New(isolate));
          if (!maybe_props.IsEmpty()) {
            props = maybe_props.ToLocalChecked();
            for(uint32_t i=0; i < props->Length(); i++)
              qpRepo_->Set(props->Get(i)->ToString(), qp_->Get(props->Get(i)->ToString())->ToNumber());
          }
        }
        qpRepo.Reset(isolate, qpRepo_);
        UI::uiSnap(uiTXT::QuotingParametersChange, &onSnap);
        UI::uiHand(uiTXT::QuotingParametersChange, &onHand);
        EV::evUp("QuotingParameters", qpRepo_);
        NODE_SET_METHOD(exports, "qpRepo", QP::_qpRepo);
      }
      static void _qpRepo(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        args.GetReturnValue().Set(Local<Object>::New(isolate, qpRepo));
      };
      static Local<Value> onSnap(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Local<Object>::New(isolate, qpRepo));
        return k;
      };
      static Local<Value> onHand(Local<Value> o_) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Object> o = o_->ToObject();
        if (o->Get(FN::v8S("buySize"))->NumberValue() > 0
          && o->Get(FN::v8S("sellSize"))->NumberValue() > 0
          && o->Get(FN::v8S("buySizePercentage"))->NumberValue() > 0
          && o->Get(FN::v8S("sellSizePercentage"))->NumberValue() > 0
          && o->Get(FN::v8S("widthPing"))->NumberValue() > 0
          && o->Get(FN::v8S("widthPong"))->NumberValue() > 0
          && o->Get(FN::v8S("widthPingPercentage"))->NumberValue() > 0
          && o->Get(FN::v8S("widthPongPercentage"))->NumberValue() > 0
        ) {
          if ((mQuotingMode)o->Get(FN::v8S("widthPongPercentage"))->NumberValue() == mQuotingMode::Depth)
            o->Set(FN::v8S("widthPercentage"), Boolean::New(isolate, false));
          qpRepo.Reset(isolate, o);
          DB::insert(uiTXT::QuotingParametersChange, o);
          EV::evUp("QuotingParameters", o);
        }
        UI::uiSend(isolate, uiTXT::QuotingParametersChange, o);
        return (Local<Value>)Undefined(isolate);
      };
  };
}

#endif

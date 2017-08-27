#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  typedef json (*qeMode)(double widthPing, double buySize, double sellSize);
  map<mQuotingMode, qeMode> qeQuote;
  class QE {
    public:
      static void main(Local<Object> exports) {
        qeQuote[mQuotingMode::Top] = &calcTopOfMarket;
        qeQuote[mQuotingMode::Mid] = &calcMidOfMarket;
        qeQuote[mQuotingMode::Join] = &calcTopOfMarket;
        qeQuote[mQuotingMode::InverseJoin] = &calcTopOfMarket;
        qeQuote[mQuotingMode::InverseTop] = &calcInverseTopOfMarket;
        qeQuote[mQuotingMode::PingPong] = &calcTopOfMarket;
        qeQuote[mQuotingMode::Boomerang] = &calcTopOfMarket;
        qeQuote[mQuotingMode::AK47] = &calcTopOfMarket;
        qeQuote[mQuotingMode::HamelinRat] = &calcTopOfMarket;
        qeQuote[mQuotingMode::Depth] = &calcDepthOfMarket;
        NODE_SET_METHOD(exports, "qeQuote", QE::_qeQuote);
      }
    private:
      static void _qeQuote(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, calc(args[0]->NumberValue(), args[1]->NumberValue(), args[2]->NumberValue()).dump())).ToLocalChecked());
      };
      static json calc(double widthPing, double buySize, double sellSize) {
        mQuotingMode k = (mQuotingMode)qpRepo["mode"].get<int>();
        if (qeQuote.find(k) == qeQuote.end()) { cout << FN::uiT() << "Errrror: Invalid quoting mode." << endl; exit(1); }
        return (*qeQuote[k])(widthPing, buySize, sellSize);
      };
      static json quoteAtTopOfMarket() {
        json topBid = mGWmktFilter["/bids/0/size"_json_pointer].get<double>() > gw->minTick
          ? mGWmktFilter["/bids/0"_json_pointer] : mGWmktFilter["/bids/1"_json_pointer];
        json topAsk = mGWmktFilter["/asks/0/size"_json_pointer].get<double>() > gw->minTick
          ? mGWmktFilter["/asks/0"_json_pointer] : mGWmktFilter["/asks/1"_json_pointer];
        return {
          {"bidPx", topBid["price"].get<double>()},
          {"bidSz", topBid["size"].get<double>()},
          {"askPx", topAsk["price"].get<double>()},
          {"askSz", topAsk["size"].get<double>()}
        };
      };
      static json calcTopOfMarket(double widthPing, double buySize, double sellSize) {
        json k = quoteAtTopOfMarket();
        if ((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Join and k["bidSz"].get<double>() > 0.2)
          k["bidPx"] = k["bidPx"].get<double>() + gw->minTick;
        k["bidPx"] = fmin(mgFairValue - widthPing / 2.0, k["bidPx"].get<double>());
        if ((mQuotingMode)qpRepo["mode"].get<int>() != mQuotingMode::Join && k["askSz"].get<double>() > 0.2)
          k["askPx"] = k["askPx"].get<double>() - gw->minTick;
        k["askPx"] = fmin(mgFairValue + widthPing / 2.0, k["askPx"].get<double>());
        k["bidSz"] = buySize;
        k["askSz"] = sellSize;
        return k;
      };
      static json calcInverseTopOfMarket(double widthPing, double buySize, double sellSize) {
        json k = quoteAtTopOfMarket();
        double mktWidth = abs(k["askPx"].get<double>() - k["bidPx"].get<double>());
        if (mktWidth > widthPing) {
          k["askPx"] = k["askPx"].get<double>() + widthPing;
          k["bidPx"] = k["bidPx"].get<double>() - widthPing;
        }
        if ((mQuotingMode)qpRepo["mode"].get<int>() == mQuotingMode::InverseTop) {
          if (k["bidSz"].get<double>() > .2) k["bidPx"] = k["bidPx"].get<double>() + gw->minTick;
          if (k["askSz"].get<double>() > .2) k["askPx"] = k["askPx"].get<double>() - gw->minTick;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k["askPx"] = k["askPx"].get<double>() + widthPing / 4.0;
          k["bidPx"] = k["bidPx"].get<double>() - widthPing / 4.0;
        }
        k["bidSz"] = buySize;
        k["askSz"] = sellSize;
        return k;
      };
      static json calcMidOfMarket(double widthPing, double buySize, double sellSize) {
        return {
          {"bidPx", fmax(mgFairValue - widthPing, 0)},
          {"bidSz", buySize},
          {"askPx", mgFairValue + widthPing},
          {"askSz", sellSize}
        };
      };
      static json calcDepthOfMarket(double depth, double buySize, double sellSize) {
        double bidPx = mGWmktFilter["/bids/0/price"_json_pointer].get<double>();
        double bidDepth = 0;
        for (json::iterator it = mGWmktFilter["bids"].begin(); it != mGWmktFilter["bids"].end(); ++it) {
          bidDepth += (*it)["size"].get<double>();
          if (bidDepth >= depth) break;
          else bidPx = (*it)["price"].get<double>();
        }
        double askPx = mGWmktFilter["/asks/0/price"_json_pointer].get<double>();
        double askDepth = 0;
        for (json::iterator it = mGWmktFilter["asks"].begin(); it != mGWmktFilter["asks"].end(); ++it) {
          askDepth += (*it)["size"].get<double>();
          if (askDepth >= depth) break;
          else askPx = (*it)["price"].get<double>();
        }
        return {
          {"bidPx", bidPx},
          {"bidSz", buySize},
          {"askPx", askPx},
          {"askSz", sellSize}
        };
      };
  };
}

#endif

#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        qp = &((CF*)config)->qp;
        json k = ((DB*)memory)->load(uiTXT::QuotingParametersChange);
        if (k.size()) {
          *qp = k.at(0);
          FN::log("DB", "loaded Quoting Parameters OK");
        } else FN::logWar("QP", "using default values for Quoting Parameters");
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::QuotingParametersChange, &hello);
        ((UI*)client)->clickme(uiTXT::QuotingParametersChange, &kiss);
      };
      void run() {
        ((UI*)client)->delay(qp->delayUI);
      };
    private:
      function<json()> hello = [&]() {
        return (json){ *qp };
      };
      function<void(json)> kiss = [&](json k) {
        if (k.value("buySize", 0.0) > 0
          and k.value("sellSize", 0.0) > 0
          and k.value("buySizePercentage", 0.0) > 0
          and k.value("sellSizePercentage", 0.0) > 0
          and k.value("widthPing", 0.0) > 0
          and k.value("widthPong", 0.0) > 0
          and k.value("widthPingPercentage", 0.0) > 0
          and k.value("widthPongPercentage", 0.0) > 0
        ) {
          *qp = k;
          ((DB*)memory)->insert(uiTXT::QuotingParametersChange, *qp);
          ((EV*)events)->uiQuotingParameters();
          ((UI*)client)->delay(qp->delayUI);
        }
        ((UI*)client)->send(uiTXT::QuotingParametersChange, *qp);
      };
  };
}

#endif

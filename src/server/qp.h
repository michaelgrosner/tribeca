#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        json k = ((DB*)memory)->load(uiTXT::QuotingParameters);
        if (k.size()) {
          *qp = k.at(0);
          FN::log("DB", "loaded Quoting Parameters OK");
        } else FN::logWar("QP", "using default values for Quoting Parameters");
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::QuotingParameters, &hello);
        ((UI*)client)->clickme(uiTXT::QuotingParameters, &kiss);
      };
      void run() {
        ((UI*)client)->delayme(qp->delayUI);
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
          ((DB*)memory)->insert(uiTXT::QuotingParameters, *qp);
          ((EV*)events)->uiQuotingParameters();
          ((UI*)client)->delayme(qp->delayUI);
        }
        ((UI*)client)->send(uiTXT::QuotingParameters, *qp);
      };
  };
}

#endif

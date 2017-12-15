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
      function<void(json*)> hello = [&](json *welcome) {
        *welcome = { *qp };
      };
      function<void(json)> kiss = [&](json butterfly) {
        *qp = butterfly;
        ((EV*)events)->uiQuotingParameters();
        ((UI*)client)->delayme(qp->delayUI);
        ((UI*)client)->send(uiTXT::QuotingParameters, *qp);
        ((DB*)memory)->insert(uiTXT::QuotingParameters, *qp);
      };
  };
}

#endif

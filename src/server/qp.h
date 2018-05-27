#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        json k = sqlite->select(mMatter::QuotingParameters);
        if (k.empty()) return screen->logWar("QP", "using default values for Quoting Parameters");
        qp = k.at(0);
        screen->log("DB", "loaded Quoting Parameters OK");
      };
      void waitUser() {
        client->welcome(mMatter::QuotingParameters, &hello);
        client->clickme(mMatter::QuotingParameters, &kiss);
      };
    private:
      function<void(json*)> hello = [&](json *welcome) {
        *welcome = { qp };
      };
      function<void(const json&)> kiss = [&](const json &butterfly) {
        mQuotingParams prev(qp);
        (qp = butterfly).diff(prev);
        engine->calcQuoteAfterSavedParams();
        client->send(mMatter::QuotingParameters, qp);
        sqlite->insert(mMatter::QuotingParameters, qp);
      };
  };
}

#endif

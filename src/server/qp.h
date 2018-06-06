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
      void waitWebAdmin() {
        client->WELCOME(mMatter::QuotingParameters, hello);
        client->CLICKME(mMatter::QuotingParameters, kiss);
      };
    private:
      void hello(json *const welcome) {
        *welcome = { qp };
      };
      void kiss(const json &butterfly) {
        mQuotingParams prev = qp;
        (qp = butterfly).diff(prev);
        engine->calcQuoteAfterSavedParams();
        client->send(mMatter::QuotingParameters, qp);
        sqlite->insert(mMatter::QuotingParameters, qp);
      };
  };
}

#endif

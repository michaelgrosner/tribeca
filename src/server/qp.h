#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        sqlite->select(
          FROM mMatter::QuotingParameters
          INTO qp
          THEN "loaded last % OK"
          WARN "using default values for %"
        );
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

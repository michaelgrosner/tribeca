#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        sqlite->backup(
          INTO qp
          THEN "loaded last % OK"
          WARN "using default values for %"
        );
      };
      void waitWebAdmin() {
        client->welcome(qp);
        client->CLICKME(qp, kiss);
      };
    private:
      void kiss(const json &butterfly) {
        qp.send_push_diff(butterfly);
        engine->calcQuoteAfterSavedParams();
      };
  };
}

#endif

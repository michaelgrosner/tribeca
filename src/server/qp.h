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
        client->WELCOME(qp, hello);
        client->CLICKME(qp, kiss);
      };
    private:
      void hello(json *const welcome) {
        *welcome = { qp };
      };
      void kiss(const json &butterfly) {
        qp.send_push_diff(butterfly);
        engine->calcQuoteAfterSavedParams();
      };
  };
}

#endif

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
        client->clickme(qp KISS {
          engine->calcQuoteAfterSavedParams();
        });
      };
  };
}

#endif

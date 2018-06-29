#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  class QP: public Klass {
    protected:
      void load() {
        sqlite->backup(&qp);
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

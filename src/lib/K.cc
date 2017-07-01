#include <nan.h>
#include "stdev.h"
#include "sqlite.h"

namespace K {
  NAN_MODULE_INIT(main) {
    Stdev::main(target);
    SQLite::main(target);
  }

  NODE_MODULE(K, main)
}

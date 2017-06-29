#include <nan.h>
#include "stdev.h"
#include "sqlite.h"

namespace K {
  NAN_MODULE_INIT(InitAll) {
    Stdev::Init(target);
    SQLite::Init(target);
  }

  NODE_MODULE(K, InitAll)
}

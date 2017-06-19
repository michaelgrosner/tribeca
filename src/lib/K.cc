#include <nan.h>
#include "stdev.h"

namespace K {
  NAN_MODULE_INIT(InitAll) {
    Stdev::Init(target);
  }

  NODE_MODULE(K, InitAll)
}

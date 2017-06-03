#include <nan.h>
#include "round.h"
#include "stdev.h"

namespace ctubio {
  NAN_MODULE_INIT(InitAll) {
    Round::Init(target);
    Stdev::Init(target);
  }

  NODE_MODULE(ctubio, InitAll)
}
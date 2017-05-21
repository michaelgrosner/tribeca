#include <nan.h>
#include "round.h"

namespace tribeca {
  NAN_MODULE_INIT(InitAll){
    Round::Init(target);
  }

  NODE_MODULE(tribeca, InitAll);
}
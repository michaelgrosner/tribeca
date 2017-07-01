#include <nan.h>
#include "stdev.h"
#include "sqlite.h"
#include "ui.h"

namespace K {
  NAN_MODULE_INIT(main) {
    Stdev::main(target);
    SQLite::main(target);
    UI::main(target);
  }

  NODE_MODULE(K, main)
}

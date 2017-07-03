#include "stdev.h"
#include "sqlite.h"
#include "ui.h"

namespace K {
  void main(Local<Object> exports) {
    Stdev::main(exports);
    SQLite::main(exports);
    UI::main(exports);
  }
}

NODE_MODULE(K, K::main)


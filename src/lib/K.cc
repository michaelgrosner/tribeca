#include <iostream>
#include <string.h>
#include <math.h>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <sqlite3.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <uv.h>
#include <uWS/uWS.h>

using namespace std;
using namespace v8;

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


#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <math.h>
#include <algorithm>
#include <map>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <sqlite3.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <uv.h>
#include <uWS.h>

using namespace std;
using namespace v8;

#include "_b64.h"

#include "ev.h"
#include "sd.h"
#include "db.h"
#include "ui.h"

namespace K {
  void main(Local<Object> exports) {
    EV::main(exports);
    SD::main(exports);
    DB::main(exports);
    UI::main(exports);
  }
}

NODE_MODULE(K, K::main)

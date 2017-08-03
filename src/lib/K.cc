#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <locale>
#include <math.h>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <map>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <curl/curl.h>

#include <sqlite3.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <uv.h>
#include <uWS.h>

using namespace std;
using namespace v8;

#include <png.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "json.h"
#include "_dec.h"
#include "_b64.h"

using namespace nlohmann;
using namespace dec;

#include "fn.h"
#include "ev.h"
#include "km.h"
#include "sd.h"
#include "kv.h"
#include "cf.h"
#include "db.h"
#include "ui.h"
#include "qp.h"
#include "mg.h"
#include "og.h"
#include "gw.h"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;namespace K {;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;void main(Local<Object> exports) {;;
;;;;;;EV::main(exports);;;;;;;;;;;;;;;;;
;;;;;;SD::main(exports);;    ;;;;    ;;;
;;;;;;CF::main(exports);;    ;;    ;;;;;
;;;;;;DB::main(exports);;        ;;;;;;;
;;;;;;UI::main(exports);;        ;;;;;;;
;;;;;;QP::main(exports);;    ;;    ;;;;;
;;;;;;MG::main(exports);;    ;;;;    ;;;
;;;;;;OG::main(exports);;    ;;;;    ;;;
;;;;;;GW::main(exports);;;;;;;;;;    ;;;
;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NODE_MODULE(K, K::main)

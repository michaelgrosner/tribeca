#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <locale>
#include <time.h>
#include <math.h>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <map>

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <sqlite3.h>
#include "quickfix/Application.h"
#include "quickfix/FileStore.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/FileLog.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <uv.h>
#include <uWS.h>

using namespace std;
using namespace v8;

#include "png.h"
#include "json.h"
#include "_dec.h"
#include "_b64.h"

using namespace nlohmann;
using namespace dec;

#include "km.h"
#include "fn.h"
#include "ev.h"
#include "cf.h"
#include "db.h"
#include "ui.h"
#include "qp.h"
#include "og.h"
#include "mg.h"
#include "pg.h"
#include "qe.h"
#include "gw.h"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;namespace K {;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;void main(Local<Object> exports) {;;
;;;;;;EV::main(exports);;;;;;;;;;;;;;;;;
;;;;;;UI::main(exports);;    ;;;;    ;;;
;;;;;;DB::main(exports);;    ;;    ;;;;;
;;;;;;QP::main(exports);;        ;;;;;;;
;;;;;;OG::main(exports);;        ;;;;;;;
;;;;;;MG::main(exports);;    ;;    ;;;;;
;;;;;;PG::main(exports);;    ;;;;    ;;;
;;;;;;QE::main(exports);;    ;;;;    ;;;
;;;;;;GW::main(exports);;;;;;;;;;    ;;;
;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NODE_MODULE(K, K::main)

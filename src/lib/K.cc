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
#include <signal.h>
#include <execinfo.h>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>

#include <sqlite3.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "quickfix/Application.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/FileStore.h"
#include "quickfix/FileLog.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"

#include <node.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <uv.h>
#include <uWS.h>

using namespace std;

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
;;;;void main(v8::Local<v8::Object> exports) {;;
;;;;;;EV::main();;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;UI::main();;;;;    ;;;;    ;;;;;;;
;;;;;;DB::main();;;;;    ;;    ;;;;;;;;;
;;;;;;QP::main();;;;;        ;;;;;;;;;;;
;;;;;;OG::main();;;;;        ;;;;;;;;;;;
;;;;;;MG::main();;;;;    ;;    ;;;;;;;;;
;;;;;;PG::main();;;;;    ;;;;    ;;;;;;;
;;;;;;QE::main();;;;;    ;;;;    ;;;;;;;
;;;;;;GW::main();;;;;;;;;;;;;    ;;;;;;;
;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NODE_MODULE(K, K::main)

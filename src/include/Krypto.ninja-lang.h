#ifndef K_LANG_H_
#define K_LANG_H_

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <random>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <locale>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <algorithm>
#include <functional>

#ifdef _WIN32
#define strsignal to_string
#else
#include <execinfo.h>
#endif

using namespace std;

#include <json.h>
#include <sqlite3.h>
#include <uWS/uWS.h>
#include <curl/curl.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include <ncurses/ncurses.h>
#include <quickfix/NullStore.h>
#include <quickfix/Application.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/OrderCancelRequest.h>

using namespace nlohmann;

#ifndef M_PI_2
#define M_PI_2 1.5707963267948965579989817342720925807952880859375
#endif

#define mClock  unsigned long long
#define mPrice  double
#define mAmount double
#define mRandId string
#define mCoinId string

#define Tclock  chrono::system_clock::now()
#define Tstamp  chrono::duration_cast<chrono::milliseconds>( \
                  Tclock.time_since_epoch()                  \
                ).count()

#define numsAz "0123456789"                 \
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
               "abcdefghijklmnopqrstuvwxyz"

#define TRUEONCE(k) (k ? !(k = !k) : k)

#define ROUND(k, x) (round((k) / x) * x)

#define private_ref private

#endif

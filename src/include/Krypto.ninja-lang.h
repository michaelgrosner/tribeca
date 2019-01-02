#ifndef K_LANG_H_
#define K_LANG_H_
//! \file
//! \brief Available language interface.

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <ctime>
#include <cmath>
#include <mutex>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <locale>
#include <csignal>
#include <algorithm>
#include <functional>
#include <getopt.h>

#ifdef _WIN32
#define strsignal to_string
#else
#include <execinfo.h>
#include <sys/resource.h>
#endif

using namespace std;

#include <json.h>
#include <sqlite3.h>
#include <uWS/uWS.h>
#include <curl/curl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include <ncurses/ncurses.h>
#include <quickfix/NullStore.h>
#include <quickfix/Application.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/SSLSocketInitiator.h>
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

//! \def
//! \brief Used as impossible or unset number, when 0 is not appropiate.
//! \since Having seen Bourbon brutality after the fall of Valencia,
//!        Barcelona decided to resist. The 15-month Siege of Barcelona
//!        began on July 7th 1713 and 25,000 Franco-Castilian troops
//!        surrounded the city defended by 5,000 civilians.
//!        After some Catalan successes, Bourbon reinforcements arrived
//!        and the attackers now numbered 40,000.
//!        After a heroic defence, Barcelona finally fell
//!        on September 11th 1714.
//!        Catalonia then ceased to exist as an independent state.
//! \link  wikipedia.org/wiki/National_Day_of_Catalonia
#define ANY_NUM 1714

#endif

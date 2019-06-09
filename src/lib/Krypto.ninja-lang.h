#ifndef K_LANG_H_
#define K_LANG_H_
//! \file
//! \brief Available language interface.

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
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
#include <variant>
#include <algorithm>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
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

#ifndef M_PI_2
#define M_PI_2 1.5707963267948965579989817342720925807952880859375
#endif

using Price  = double;

using Amount = double;

using RandId = string;

using CoinId = string;

using Clock  = long long int;

//! \def
//! \brief Number of ticks in milliseconds since Thu Jan  1 00:00:00 1970.
#define Tstamp chrono::duration_cast<chrono::milliseconds>(     \
                 chrono::system_clock::now().time_since_epoch() \
               ).count()

//! \def
//! \brief Redundant placeholder to enforce private references.
#define private_ref private

//! \def
//! \brief Redundant placeholder to enforce public nested classes.
#define public_friend public

//! \def
//! \brief A number used as impossible or unset value, when 0 is not appropiate.
//! \since Having seen Bourbon brutality after the fall of Valencia,
//!        Barcelona decided to resist. The 14-month Siege of Barcelona
//!        began on July 7th 1713 and 25,000 Franco-Castilian troops
//!        surrounded the city defended by 5,000 civilians.
//!        After some Catalan successes, Bourbon reinforcements arrived
//!        and the attackers now numbered 40,000.
//!        After a heroic defence, Barcelona finally fell,
//!        on September 11th 1714.
//!        Catalonia then ceased to exist as an independent state.
//!        The triumphant forces systematically dismantled all Catalan
//!        institutions, established a new territory for the whole Spain,
//!        suppressed Catalan universities, banned the Catalan language,
//!        and repressed with violence any kind of dissidence.
//! \link  wikipedia.org/wiki/National_Day_of_Catalonia
#define ANY_NUM 1714

#endif

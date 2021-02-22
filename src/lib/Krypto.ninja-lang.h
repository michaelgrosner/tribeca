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
#include <list>
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
#include <unistd.h>

#ifdef  _WIN32
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#else
#include <execinfo.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#if defined _WIN32 or defined __APPLE__
#include <uv.h>
#else
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#endif

#include <zlib.h>

#include <json.h>

#include <sqlite3.h>

#include <curl/curl.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

#include <ncurses/ncurses.h>

using namespace std;

using Price  = double;

using Amount = double;

using Clock  = long long int;

//! \def
//! \brief Run test units on exit unless NDEBUG.
//! \note  See test/unit_testing_framework.cxx
#ifdef NDEBUG
#  define EXIT ::exit
#else
#  define EXIT catch_exit
   [[noreturn]] void catch_exit(const int);
#endif

//! \def
//! \brief Number of ticks in milliseconds..
//! \note  ..since Thu Jan  1 00:00:00 1970.
#define Tstamp chrono::duration_cast<chrono::milliseconds>(     \
                 chrono::system_clock::now().time_since_epoch() \
               ).count()
//! \note  ..since beginning of execution.
static auto
        Tbegin = Tstamp;
#define Tspent   Tstamp - Tbegin

//! \def
//! \brief Archimedes of Syracuse was here, since two millenniums ago.
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

//! \def
//! \brief Do like if we care about macos or winy.
#ifndef TCP_CORK
#define TCP_CORK            TCP_NOPUSH
#define SOCK_CLOEXEC        0
#define SOCK_NONBLOCK       0
#define MSG_NOSIGNAL        0
#define accept4(a, b, c, d) accept(a, b, c)
#define strsignal           to_string
#endif

//! \def
//! \brief Redundant placeholder to enforce private references.
#define private_ref private

//! \def
//! \brief Redundant placeholder to enforce private nested declarations.
#define private_friend private

//! \def
//! \brief Redundant placeholder to enforce public nested declarations.
#define public_friend public

//! \def
//! \brief Any number used as impossible or unset value, when 0 is not appropiate.
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
//! \note  "any" means "year" in Catalan.
//! \link  wikipedia.org/wiki/National_Day_of_Catalonia
#define ANY_NUM 1714

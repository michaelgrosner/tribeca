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
#include <conio.h>
#else
#include <termios.h>
#include <execinfo.h>
#include <sys/ioctl.h>
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

using namespace std;

using Price  = double;

using Amount = double;

using Clock  = long long int;

//! \def
//! \brief Run test units on exit unless NDEBUG.
//! \note  See test/unit_testing_framework.cxx
#ifdef NDEBUG
#define EXIT ::exit
#else
#define EXIT catch_exit
[[noreturn]] void catch_exit(const int);
#endif

//! \def
//! \brief Number of ticks in milliseconds.
//! \since Thu Jan  1 00:00:00 1970.
#define Tstamp chrono::duration_cast<chrono::milliseconds>(     \
                 chrono::system_clock::now().time_since_epoch() \
               ).count()
//! \since Beginning of execution.
static auto
        Tbegin = Tstamp;
#define Tspent   Tstamp - Tbegin

//! \def
//! \brief Archimedes of Syracuse was here.
//! \since Two millenniums ago.
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

//! \def
//! \brief I like turtles.
static bool colorful = true;
#define ANSI_BLACK          "0"
#define ANSI_RED            "1"
#define ANSI_GREEN          "2"
#define ANSI_YELLOW         "3"
#define ANSI_BLUE           "4"
#define ANSI_MAGENTA        "5"
#define ANSI_CYAN           "6"
#define ANSI_WHITE          "7"
#define ANSI_COLOR(a)       string(colorful ? a : "")
#define ANSI_COLORS_SIZE(a) (colorful ? a * 7 : 0 )
#define ANSI_SYMBOL_SIZE(a) (a * 2)
#define ANSI_ESC            "\x1b"
#define ANSI_BELL           "\007"
#define ANSI_NEW_LINE       "\r\n"
#define ANSI_CODE           ANSI_ESC "["
#define ANSI_CURSOR         ANSI_CODE "1 q"
#define ANSI_RESET          ANSI_CODE "0m"
#define ANSI_CLEAR_ROW      ANSI_CODE "0K"
#define ANSI_CLEAR_ROWS     ANSI_CODE "2J"
#define ANSI_END_LINE       ANSI_CLEAR_ROW ANSI_NEW_LINE
#define ANSI_SHOW_CURSOR    ANSI_CODE "?25h"
#define ANSI_HIDE_CURSOR    ANSI_CODE "?25l"
#define ANSI_PUKE(a)        ANSI_CODE "0;3" a "m"
#define ANSI_HIGH(a)        ANSI_CODE "1;3" a "m"
#define ANSI_WAVE(a)        ANSI_CODE "5;3" a "m"
#define ANSI_MOVE(a, b)     ANSI_CODE a ";" b "H"
#define ANSI_TOP_RIGHT      ANSI_MOVE("0", "0")
#define ANSI_BUFFER         ANSI_CODE "?1049"
#define ANSI_ALTERNATIVE    ANSI_BUFFER "h" ANSI_CLEAR_ROWS
#define ANSI_ORIGINAL       ANSI_CLEAR_ROWS ANSI_BUFFER "l"
#define ANSI_PUKE_BLACK     ANSI_COLOR(ANSI_PUKE(ANSI_BLACK))
#define ANSI_PUKE_RED       ANSI_COLOR(ANSI_PUKE(ANSI_RED))
#define ANSI_PUKE_GREEN     ANSI_COLOR(ANSI_PUKE(ANSI_GREEN))
#define ANSI_PUKE_YELLOW    ANSI_COLOR(ANSI_PUKE(ANSI_YELLOW))
#define ANSI_PUKE_BLUE      ANSI_COLOR(ANSI_PUKE(ANSI_BLUE))
#define ANSI_PUKE_MAGENTA   ANSI_COLOR(ANSI_PUKE(ANSI_MAGENTA))
#define ANSI_PUKE_CYAN      ANSI_COLOR(ANSI_PUKE(ANSI_CYAN))
#define ANSI_PUKE_WHITE     ANSI_COLOR(ANSI_PUKE(ANSI_WHITE))
#define ANSI_HIGH_BLACK     ANSI_COLOR(ANSI_HIGH(ANSI_BLACK))
#define ANSI_HIGH_RED       ANSI_COLOR(ANSI_HIGH(ANSI_RED))
#define ANSI_HIGH_GREEN     ANSI_COLOR(ANSI_HIGH(ANSI_GREEN))
#define ANSI_HIGH_YELLOW    ANSI_COLOR(ANSI_HIGH(ANSI_YELLOW))
#define ANSI_HIGH_BLUE      ANSI_COLOR(ANSI_HIGH(ANSI_BLUE))
#define ANSI_HIGH_MAGENTA   ANSI_COLOR(ANSI_HIGH(ANSI_MAGENTA))
#define ANSI_HIGH_CYAN      ANSI_COLOR(ANSI_HIGH(ANSI_CYAN))
#define ANSI_HIGH_WHITE     ANSI_COLOR(ANSI_HIGH(ANSI_WHITE))
#define ANSI_WAVE_BLACK     ANSI_COLOR(ANSI_WAVE(ANSI_BLACK))
#define ANSI_WAVE_RED       ANSI_COLOR(ANSI_WAVE(ANSI_RED))
#define ANSI_WAVE_GREEN     ANSI_COLOR(ANSI_WAVE(ANSI_GREEN))
#define ANSI_WAVE_YELLOW    ANSI_COLOR(ANSI_WAVE(ANSI_YELLOW))
#define ANSI_WAVE_BLUE      ANSI_COLOR(ANSI_WAVE(ANSI_BLUE))
#define ANSI_WAVE_MAGENTA   ANSI_COLOR(ANSI_WAVE(ANSI_MAGENTA))
#define ANSI_WAVE_CYAN      ANSI_COLOR(ANSI_WAVE(ANSI_CYAN))
#define ANSI_WAVE_WHITE     ANSI_COLOR(ANSI_WAVE(ANSI_WHITE))

//! \def
//! \brief Do like if we care about macos or winy.
#ifndef  TCP_CORK
#define  TCP_CORK            TCP_NOPUSH
#define  SOCK_CLOEXEC        0
#define  SOCK_NONBLOCK       0
# ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL        0
# endif
#define  accept4(a, b, c, d) accept(a, b, c)
#define  strsignal           to_string
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

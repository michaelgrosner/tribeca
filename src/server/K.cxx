#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
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
#include <functional>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>

#ifndef _WIN32
#  include <execinfo.h>
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

#define PERMISSIVE_analpaper_SOFTWARE_LICENSE                              \
                                                                           \
       "This is free software: the UI and quoting engine are open source," \
"\n"   "feel free to hack both as you need."                               \
                                                                           \
"\n"   "This is non-free software: built-in gateway exchange integrations" \
"\n"   "are licensed by/under the law of my grandma (since last century)," \
"\n"   "feel free to crack all as you need."

#ifdef NDEBUG
#  define EXIT exit
#else
#  include <catch.h>
#  define EXIT catch_exit
   void catch_exit(const int);
#endif

#include "ds.h"
#include "if.h"
#include "sh.h"
#include "ev.h"
#include "db.h"
#include "ui.h"
#include "qe.h"

#ifndef NDEBUG
#  include <test/units.h>
#endif

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;;int main(int argc, char** argv) {;;
    ;;;;K::SH sh;;;;;;;;;;;;;;;;;;;;;;;;;
    ;;;;K::EV ev;;;;;;;    ;;;;    ;;;;;;
    ;;;;K::DB db;;;;;;;    ;;    ;;;;;;;;
    ;;;;K::UI ui;;;;;;;        ;;;;;;;;;;
    ;;;;K::QE qe;;;;;;;        ;;;;;;;;;;
    ;;/*|       */;;;;;    ;;    ;;;;;;;;
;   ;;/*   o    */;;;;;    ;;;;    ;;;;;;           ;
;;  ;;/*       |*/;;;;;    ;;;;    ;;;;;;          ;;
;;;;;;/*        */;;;;;;;;;;;;;    ;;;;;;    ;;;;;;;; // youtu.be/dVlGmdl-g9Q
;;;;;;;;sh.main(argc, argv);;;;;;;;;;;;;;;;;;;;;;
;   ;;;;ev.wait(/*  Wherever you go..     */);;;;;;;; // youtu.be/DKSO5YlYbOg
    ;;;;db.wait(/*  Whatever you do..     */);;;;
    ;;;;ui.wait(/*  I will be right here  */);;;;;;;; // youtu.be/FornpYhezt4
    ;;;;qe.wait(/*   waiting for coins.   */);;;;
;   ;;/* .-""-._    Whatever it takes..   **/;;;;;;;; // youtu.be/Wd2fSSt4MDg
;   ;;/*(  <>  )`-. Or how my OS breaks,  **/;;;;
;   ;;/*|`-..-'|  | I will be right here  **/;;;;;;;; // youtu.be/02OHHWG1EQY
;   ;;/*|     :|._/  waiting for coins.   **/;;;;
;   ;;/*`.____;'    Day after day.     :wq**/;;;;;;;; // youtu.be/AMCeEoOgSvc
;;  ;;;;return EXIT_FAILURE;;;;;;;;;;;;;;;;;;;;;
;;;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; // youtu.be/dp5hsDgENLk
;;  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;   ;;
;                        /*K\*/                     ;
         /*    .        kKKKKK,       .         *
                       kKKKK  KK,
     .         ,kFIREKKKKLET  `KK,_         *
          ,RUNNERV'      KKKK   `K,`\
        ,KKK    KK   .    KTHE   `V
     kKKKV'     KK         KKKK    \_              .
*    V   l   .   KK        KKKFIRE         .
     l    \       KK,     FKK  KKKKKK,
    /            KK l    IKK    RUN `K,
                KK  l   RKK     K K, `K,          *
      .        KK      EKK      K  K, `l
               K        KK      V  `K   \
               V        RKK     l   V
 K             l        V Uk        l      .
  \             \       l  Nk
                  \         K     *    X
                       \    V    /    /        K
                 X   \  \ X |  / X  /        /
                  \       \\|/ /  /
_____ youtu.be/he9MKrsZTN8 \|/ youtu.be/l13OfmQlov8 _____
\* ### youtu.be/w4xWGNQwJCs x youtu.be/LC-W2YV9Tbc ### */

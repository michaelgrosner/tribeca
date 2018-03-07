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
#include <execinfo.h>
#endif

using namespace std;

#include "json.h"
#include "sqlite3.h"
#include "uWS/uWS.h"
#include "curl/curl.h"
#include "openssl/md5.h"
#include "openssl/bio.h"
#include "openssl/evp.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include "openssl/buffer.h"
#include "ncurses/ncurses.h"
#include "quickfix/NullStore.h"
#include "quickfix/Application.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/OrderCancelRequest.h"

using namespace nlohmann;

#include "fn.h"
#include "km.h"
#include "sh.h"
#include "cf.h"
#include "ev.h"
#include "db.h"
#include "ui.h"
#include "qp.h"
#include "og.h"
#include "mg.h"
#include "pg.h"
#include "qe.h"
#include "gw.h"

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;;int main(int argc, char** argv) {;;
    ;;;;K::CF cf;;;;;;;;;;;;;;;;;;;;;;;;;
    ;;;;K::EV ev;;;;;;;    ;;;;    ;;;;;;
    ;;;;K::DB db;;;;;;;    ;;    ;;;;;;;;
    ;;;;K::UI ui;;;;;;;        ;;;;;;;;;;
    ;;;;K::QP qp;;;;;;;        ;;;;;;;;;;
    ;;;;K::OG og;;;;;;;    ;;    ;;;;;;;;
;   ;;;;K::MG mg;;;;;;;    ;;;;    ;;;;;;           ;
;;  ;;;;K::PG pg;;;;;;;    ;;;;    ;;;;;;          ;;
;;;;;;;;K::QE qe;;;;;;;;;;;;;;;    ;;;;;;        ;;;;;;;; // youtu.be/dVlGmdl-g9Q
    ;;;;K::GW gw;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;cf.main(argc, argv);;;;;;;;;;;;;;;;;;;;;;;;;;;;;; // youtu.be/nwyDU3SGgQQ
;;  ;;  cf.link(ev, db, ui, qp, og, mg, pg, qe, gw);;
;   ;;  ev.wait(/*     Wherever you go..        */);;;;;; // youtu.be/DKSO5YlYbOg
    ;;  db.wait(/*     Whatever you do..        */);;
    ;;  ui.wait(/*     I will be right here     */);;;;;; // youtu.be/FornpYhezt4
    ;;  qp.wait(/*      waiting for coins.      */);;
    ;;  og.wait(/*     Whatever it takes..      */);;;;;; // youtu.be/Wd2fSSt4MDg
    ;;  mg.wait(/*     Or how my OS breaks,     */);;
    ;;  pg.wait(/*     I will be right here     */);;;;;; // youtu.be/02OHHWG1EQY
    ;;  qe.wait(/*      waiting for coins.      */);;
;   ;;  gw.wait(/*     Day after day.        :wq*/);;;;;; // youtu.be/AMCeEoOgSvc
;;  ;;;;return EXIT_FAILURE;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ;;;;;;; // youtu.be/dp5hsDgENLk
;;  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;  ;;
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
                KK  l   RKK     K K, `K,            *
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

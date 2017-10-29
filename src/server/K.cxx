#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <locale>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <execinfo.h>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

#include "uv.h"
#include "json.h"
#include "sqlite3.h"
#include "uWS/uWS.h"
#include "curl/curl.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"
#include "openssl/md5.h"
#include "ncurses/ncurses.h"
#include "quickfix/Application.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/FileStore.h"
#include "quickfix/FileLog.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "quickfix/fix42/OrderCancelReject.h"

using namespace nlohmann;

#include "km.h"
#include "fn.h"
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
;   ;;;;K::MG mg;;;;;;;    ;;;;    ;;;;;;
;;  ;;;;K::PG pg;;;;;;;    ;;;;    ;;;;;;  ;;
;;;;;;;;K::QE qe;;;;;;;;;;;;;;;    ;;;;;; ;;;;;;;
    ;;;;K::GW gw;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;  cf.main(argc, argv)                ;;;;;;
;;  ;;  ev.wait(/* Wherever you go      */);;
;   ;;  db.wait(/* Whatever you do      */);;
    ;;  ui.wait(/* I will be right here */);;
    ;;  qp.wait(/*  waiting for coins   */);;
    ;;  og.wait(/* Whatever it takes    */);;
    ;;  mg.wait(/* Or how my OS breaks  */);;
    ;;  pg.wait(/* I will be right here */);;
    ;;  qe.wait(/*  waiting for coins   */);;
;   ;;  gw.wait(/* Day after day        */);;
;;  ;;;;return EXIT_FAILURE;;;;;;;;;;;;;;;;;;
;;;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ;;;;;;;
;;  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;  ;;
;        /*    .        kKKKKK,       .          *
                       kKKKK  KK,
     .         ,kFIREKKKKLET  `KK,_          *
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
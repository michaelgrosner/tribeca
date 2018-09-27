#ifndef K_LIB_H_
#define K_LIB_H_
//! \dir
//! \brief namespace \ref K.

//! \file
//! \brief namespace \ref K.

#define PERMISSIVE_analpaper_SOFTWARE_LICENSE                              \
                                                                           \
       "This is free software: the UI and quoting engine are open source," \
"\n"   "feel free to hack both as you need."                               \
                                                                           \
"\n"   "This is non-free software: built-in gateway exchange integrations" \
"\n"   "are licensed by/under the law of my grandma (since last century)," \
"\n"   "feel free to crack all as you need."

#ifdef NDEBUG
#  define EXIT ::exit
#else
#  include <catch.h>
#  define EXIT catch_exit
   void catch_exit(const int);
#endif

//! \namespace K
//! \brief     class \ref K::Klass, fn \ref K::exit, fn \ref K::error.

#include <Krypto.ninja-lang.h>
#include <Krypto.ninja-apis.h>
#include <Krypto.ninja-bots.h>
#include <Krypto.ninja-data.h>

#ifndef NDEBUG
#  include <../../test/units.h>
#endif

#endif

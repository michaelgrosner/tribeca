#ifndef K_LIB_H_
#define K_LIB_H_
//! \dir
//! \brief namespace \ref ₿.

//! \file
//! \brief namespace \ref ₿.

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

//! \namespace ₿
//! \brief     class \ref ₿::Klass, fn \ref ₿::exit, fn \ref ₿::error.

#include <Krypto.ninja-lang.h>
#include <Krypto.ninja-apis.h>
#include <Krypto.ninja-bots.h>
#include <Krypto.ninja-data.h>

#ifndef NDEBUG
#  include <../../test/units.h>
#endif

#endif

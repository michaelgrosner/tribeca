//! \file
//! \brief Color palette override for terminals.
//! \note  ANSI color codes to override ANSI_* colors:
//!          "0" BLACK     "4" BLUE
//!          "1" RED       "5" MAGENTA
//!          "2" GREEN     "6" CYAN
//!          "3" YELLOW    "7" WHITE

//! \def
//! \brief Define color red     as magenta.
#define ANSI_RED                   "5"

//! \def
//! \brief Define color magenta as red.
#define ANSI_MAGENTA               "1"

//! \def
//! \brief Define color green   as yellow.
#define ANSI_GREEN                 "3"

//! \def
//! \brief Define color yellow  as green.
#define ANSI_YELLOW                "2"

//! \def
//! \brief Define color cyan    as blue.
#define ANSI_CYAN                  "4"

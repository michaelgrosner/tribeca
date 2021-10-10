//! \file
//! \brief Lazy declarations OR definitions of disk files.

//! \def
//! \brief Define variable names for any disk file.
#define DISK_VARNAME(a, b) a ## _disk_file_ ## b
#ifdef __APPLE__
#define DISK_SYMBOL(a) DISK_VARNAME(_, a)
#else
#define DISK_SYMBOL(a) DISK_VARNAME( , a)
#endif

//! \def
//! \brief Lazy (assembler OR c++) code for any disk file.
#ifdef __ASSEMBLER__
#define DISK_FILE(a, b, c) .disk_file DISK_SYMBOL(a), b;
#else
#define DISK_FILE(a, b, c) extern const char DISK_VARNAME(, a);         \
                           extern const  int DISK_VARNAME(, a ## _len);
#define DISK_DOCS(a, b, c)         { #c, {  &DISK_VARNAME(, a),         \
                                             DISK_VARNAME(, a ## _len)}},
#endif

//! \def
//! \brief Define empty default DISK for headless bots.
#ifndef DISK
#define DISK(a)
#endif

//! \def
//! \brief Use whatever code for any disk file (c++).
#define DISK_WWW  \
        DISK      \
      ( DISK_DOCS )

//! \def
//! \brief Use whatever code for any disk file (assembler OR c++).
#define DISK_DATA \
        DISK      \
      ( DISK_FILE )

//! \def
//! \brief Let any disk file be declared (c++) OR defined (assembler).
#ifdef __ASSEMBLER__
    .data

  .macro       \
    .disk_file \
     var,      \
     uri
    .global    \var
    .global    \var\()_len
\var:
    .incbin    "\uri"
\var\()_len:
    .int       . - \var
  .endm
#endif

    DISK_DATA

#ifdef __ASSEMBLER__
    .end
#endif

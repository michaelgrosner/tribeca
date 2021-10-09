//! \file
//! \brief Lazy declarations OR definitions of disk files.

//! \def
//! \brief Enforce underscore prefix for global external symbol names.
#ifdef __APPLE__
#define _SYM(a) _ ## a
#else
#define _SYM(a)      a
#endif

//! \def
//! \brief Lazy (assembler OR c++) code for any disk file.
#ifdef __ASSEMBLER__
#define FILE(a, b, c)   .disk_file _SYM(_disk_file_ ## a), b;
#else
#define FILE(a, b, c) extern const char _disk_file_ ## a;         \
                      extern const  int _disk_file_ ## a ## _len;
#define DOCS(a, b, c)         { #c, {                            \
                                       &_disk_file_ ## a,        \
                                        _disk_file_ ## a ## _len \
                              }     },
#endif

//! \def
//! \brief Define empty default DISK for headless bots.
#ifndef DISK
#define DISK(a)
#endif

//! \def
//! \brief Use whatever code for any disk file (c++).
#define DISK_WWW \
        DISK     \
      ( DOCS )

//! \def
//! \brief Use whatever code for any disk file (assembler OR c++).
#define DATA \
        DISK \
      ( FILE )

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

    DATA

#ifdef __ASSEMBLER__
    .end
#endif

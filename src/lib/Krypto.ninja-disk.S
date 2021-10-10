//! \file
//! \brief Lazy declarations OR definitions of disk files.

//! \def
//! \brief Define unique variable names for any disk file.
#define  DISK_SYM(a, b) a ## _disk_file_ ## b

//! \def
//! \brief Enforce underscore prefix for global external symbol names.
#ifdef __APPLE__
#define _DISK_SYM(a) DISK_SYM(_, a)
#else
#define _DISK_SYM(a) DISK_SYM( , a)
#endif

//! \def
//! \brief Lazy (assembler OR c++) code for any disk file.
#ifdef __ASSEMBLER__
#define DISK_FILE(a, b, c)       .disk_file _DISK_SYM(a), b;
#else
#define DISK_FILE(a, b, c) extern const char DISK_SYM(, a);         \
                           extern const  int DISK_SYM(, a ## _len);
#define DISK_URLS(a, b, c)         { #c, {  &DISK_SYM(, a),         \
                                             DISK_SYM(, a ## _len)}},
#endif

//! \def
//! \brief Define empty default DISK for headless bots.
#ifndef DISK
#define DISK(a)
#endif

//! \def
//! \brief Use whatever code for any disk file (c++).
#define DISK_DOCS \
        DISK      \
      ( DISK_URLS )

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

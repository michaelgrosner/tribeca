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
//! \brief Lazy assembler or c++ code for any disk file.
#ifdef __ASSEMBLER__
#define CODE(a, b)   .disk_file _SYM(_disk_file_ ## a), b;
#else
#define CODE(a, b) extern const char _disk_file_ ## a; \
                   extern const  int _disk_file_ ## a ## _len;
#define DISK_FILE(a)             {                            \
                                    &_disk_file_ ## a,        \
                                     _disk_file_ ## a ## _len \
                                 }
#endif

//! \def
//! \brief Use whatever code for any disk file (if any DISK exists).
#ifndef DISK
#define FILE
#else
#define FILE \
        DISK \
      ( CODE )
#endif

//! \def
//! \brief Let any disk file be declared (c++) or defined (assembler).
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

    FILE

#ifdef __ASSEMBLER__
    .end
#endif

//! \def
//! \brief I'm proud of you, son.
//! \      Dad, are you space?
//! \      Yes, now we are a family again.
#undef  _SYM
#undef  DISK
#undef  FILE
#undef  CODE

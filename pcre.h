/*************************************************
 *       Perl-Compatible Regular Expressions      *
 *************************************************/

/* Copyright (c) 1997-1999 University of Cambridge */

#ifndef _PCRE_H
#define _PCRE_H

/* ICI */
#define pcre_compile ici_pcre_compile
#define pcre_exec ici_pcre_exec
#define pcre_free ici_pcre_free
#define pcre_info ici_pcre_info
#define pcre_malloc ici_pcre_malloc
#define pcre_study ici_pcre_study
#define pcre_version ici_pcre_version
/*
 * Force STATIC to be defined so pcre_malloc & pcre_free
 * don't get a dllimport declspec applied - the linker
 * doesn't like it and complains.
 */
#ifndef STATIC
#define STATIC
#endif
/* ICI */

#define PCRE_MAJOR 2
/* ICI */
/* #define PCRE_MINOR 08 */
#define PCRE_MINOR 010 /* More correct octal thankyou! (stops warnings) */
/* ICI */
#define PCRE_DATE 31 - Aug - 1999

/* Win32 uses DLL by default */

#ifdef _WIN32
#ifdef STATIC
#define PCRE_DL_IMPORT
#else
#define PCRE_DL_IMPORT __declspec(dllimport)
#endif
#else
#define PCRE_DL_IMPORT
#endif

/* Have to include stdlib.h in order to ensure that size_t is defined;
it is needed here for malloc. */

#include <stdlib.h>
#include <sys/types.h>

/* Allow for C++ users */

#ifdef __cplusplus
extern "C"
{
#endif

    /* Options */

#define PCRE_CASELESS 0x0001
#define PCRE_MULTILINE 0x0002
#define PCRE_DOTALL 0x0004
#define PCRE_EXTENDED 0x0008
#define PCRE_ANCHORED 0x0010
#define PCRE_DOLLAR_ENDONLY 0x0020
#define PCRE_EXTRA 0x0040
#define PCRE_NOTBOL 0x0080
#define PCRE_NOTEOL 0x0100
#define PCRE_UNGREEDY 0x0200
#define PCRE_NOTEMPTY 0x0400

    /* Exec-time and get-time error codes */

#define PCRE_ERROR_NOMATCH (-1)
#define PCRE_ERROR_NULL (-2)
#define PCRE_ERROR_BADOPTION (-3)
#define PCRE_ERROR_BADMAGIC (-4)
#define PCRE_ERROR_UNKNOWN_NODE (-5)
#define PCRE_ERROR_NOMEMORY (-6)
#define PCRE_ERROR_NOSUBSTRING (-7)

    /* Types */

#ifndef ICI_PCRE_TYPES_DEFINED
    typedef void pcre;
    typedef void pcre_extra;
#define ICI_PCRE_TYPES_DEFINED
#endif

    /* Store get and free functions. These can be set to alternative malloc/free
    functions if required. Some magic is required for Win32 DLL; it is null on
    other OS. */

    PCRE_DL_IMPORT extern void *(*pcre_malloc)(size_t);
    PCRE_DL_IMPORT extern void (*pcre_free)(void *);

#undef PCRE_DL_IMPORT

    /* Functions */

    extern pcre       *pcre_compile(const char *, int, const char **, int *, const unsigned char *);
    extern int         pcre_copy_substring(const char *, int *, int, int, char *, int);
    extern int         pcre_exec(const pcre *, const pcre_extra *, const char *, int, int, int, int *, int);
    extern int         pcre_get_substring(const char *, int *, int, int, const char **);
    extern int         pcre_get_substring_list(const char *, int *, int, const char ***);
    extern int         pcre_info(const pcre *, int *, int *);
    extern pcre_extra *pcre_study(const pcre *, int, const char **);
    extern const char *pcre_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* End of pcre.h */

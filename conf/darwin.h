#ifndef ICI_CONF_H
#define ICI_CONF_H

#define _DARWIN_C_SOURCE 1

#include <math.h>
#include <sys/param.h>

#define CONFIG_STR      "MacOS"

#ifndef PREFIX
#define PREFIX          "/usr/local/"
#endif

#define ICI_DLL_EXT     ".bundle"

#include <crt_externs.h>
#define environ *_NSGetEnviron()
#define ICI_HAS_BSD_STRUCT_TM

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#if 0
#define ICI_PTR_HASH(p) (((unsigned long)(p) >> 12) * 31 ^ ((unsigned long)(p) >> 4) * 17)
#endif

#endif /*ICI_CONF_H*/

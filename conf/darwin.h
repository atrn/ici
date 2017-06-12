#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <math.h>
#include <sys/param.h>

#define NOEVENTS        /* Event loop and associated processing. */
// #define NOPROFILE       /* Profiler, see profile.c. */
// #define NODEBUGGING     /* Debugger interface. */

#define CONFIG_STR      "MacOS"
#ifndef PREFIX
#define PREFIX          "/opt/anici/"
#endif
#define ICI_DLL_EXT     ".dylib"

#include <crt_externs.h>
#define environ *_NSGetEnviron()
#define ICI_HAS_BSD_STRUCT_TM

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#define ICI_PTR_HASH(p) (((unsigned long)(p) >> 12) * 31 ^ ((unsigned long)(p) >> 4) * 17)

#endif /*ICI_CONF_H*/

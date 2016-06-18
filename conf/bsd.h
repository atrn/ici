#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <unistd.h>
#include <math.h>

#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */

#define ICI_USE_POSIX_THREADS
#define ICI_HAS_BSD_STRUCT_TM
#define CONFIG_STR      "BSD UNIX"

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#endif /*ICI_CONF_H*/

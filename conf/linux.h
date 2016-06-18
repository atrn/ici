#ifndef ICI_CONF_H
#define ICI_CONF_H

#define ICI_NO_OLD_NAMES

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */

/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_USE_POSIX_THREADS
#define ICI_HAS_BSD_STRUCT_TM

// #define ICI_PTR_HASH(p) (((unsigned long)(p) >> 12) * 31 ^((unsigned long)(p) >> 4) * 17)
// #define ICI_PTR_HASH(p) ((unsigned long)(p))
// #define ICI_USE_MURMUR_HASH

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Linux"

/*
 * To have GNU libc enable various things we need to...
 */
#define _GNU_SOURCE

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#endif /*ICI_CONF_H*/

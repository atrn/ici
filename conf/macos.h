#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <math.h>
#include <sys/param.h>

#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#define NODEBUGGING     /* Debugger interface. */

#define CONFIG_STR      "MacOS"
#ifndef PREFIX
#define PREFIX          "/opt/anici/"
#endif
#define ICI_DLL_EXT     ".bundle"
#define ICI_USE_POSIX_THREADS
#define sem_t ici_sem_t
#define sem_init ici_sem_init
#define sem_destroy ici_sem_destroy
#define sem_wait ici_sem_wait
#define sem_post ici_sem_post

#include <pthread.h>

#include <crt_externs.h>
#define environ *_NSGetEnviron()
#define ICI_HAS_BSD_STRUCT_TM

#define  UNLIKELY(X) __builtin_expect((X), 0)
#define  LIKELY(X)   __builtin_expect((X), 1)

#define ICI_PTR_HASH(p) (((unsigned long)(p) >> 12) * 31 ^ ((unsigned long)(p) >> 4) * 17)

#endif /*ICI_CONF_H*/

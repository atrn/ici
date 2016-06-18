#ifndef ICI_CONF_H
#define ICI_CONF_H

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#define NOWAITFOR       /* Requires select() or similar system primitive. */
#define NOSYSTEM        /* Command interpreter (shell) escape. */
#define NOPIPES         /* Requires popen(). */
#define NODIR           /* Directory reading function, dir(). */
#define NODLOAD         /* Dynamic loading of native machine code modules. */
#define NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#define NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#define NOSIGNALS       /* ICI level signal handling */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "math"

#endif /*ICI_CONF_H*/

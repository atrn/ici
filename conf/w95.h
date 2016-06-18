#ifndef ICI_CONF_H
#define ICI_CONF_H

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#define NOWAITFOR       /* Requires select() or similar system primitive. */
#define NOPIPES         /* Requires popen(). */
#undef  NODLOAD         /* Dynamic loading of native machine code modules. */
#define NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOPROFILE       /* Profiler, see profile.c. */
#define NOSIGNALS       /* ICI level signal handling */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "math"

#endif /*ICI_CONF_H*/

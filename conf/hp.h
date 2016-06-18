#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD
#define BAD_PRINTF_RETVAL

#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#undef  NOWAITFOR       /* Requires select() or similar system primitive. */
#define NODIR           /* Directory reading function, dir(). */
#define NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#undef  NOSIGNALS       /* ICI level signal handling */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "HP-UX configuration"

#endif /*ICI_CONF_H*/

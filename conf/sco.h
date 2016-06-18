#ifndef ICI_CONF_H
#define ICI_CONF_H

#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "SCO Unix"

#endif /*ICI_CONF_H*/

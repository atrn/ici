#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43

#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "NeXTSTEP 2.x, 3.x, 4.x"

#endif /*ICI_CONF_H*/

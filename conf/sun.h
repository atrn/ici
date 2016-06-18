#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43
#define BAD_PRINTF_RETVAL

#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "SunOS 4.x"

#include <sys/param.h>
#undef ici_isset

#ifndef FILENAME_MAX
#define FILENAME_MAX    MAXPATHLEN
#endif

#endif /*ICI_CONF_H*/

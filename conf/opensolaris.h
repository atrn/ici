#ifndef ICI_CONF_H
#define ICI_CONF_H

#include <sys/param.h>
#undef ici_isset

#include <math.h>

#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */

#define CONFIG_STR      "OpenSolaris"

#endif /*ICI_CONF_H*/

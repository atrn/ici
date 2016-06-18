#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following macros control the inclusion of various ICI intrinsic
 * functions. If the macro is defined the functions are NOT include
 * (hence the NO at the start of the name). By undef'ing the macro you
 * are stating that the functions should be included and compile (and
 * possibly even work) for the particular port of ICI.
 */
#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */

/*
 * This string gets compiled into the executable.
 */
#define CONFIG_STR      "This string gets compiled into the ICI executable"

/*
 *  Does this system have the timezone-related fields in struct tm?
 *  As per BSD's struct tm these are:
 *
 *    long	tm_gmtoff;	/* offset from UTC in seconds */
 *    char	*tm_zone;	/* timezone abbreviation */
 *
 */
#undef ICI_HAS_BSD_STRUCT_TM

#endif /*ICI_CONF_H*/

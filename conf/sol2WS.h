#ifndef ICI_CONF_H
#define ICI_CONF_H

#define BSD     43
#define BAD_PRINTF_RETVAL

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#ifndef SUNOS5
#define SUNOS5
#endif

#define NOTRACE         /* For debugging. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */

#define ICI_USE_POSIX_THREADS

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Solaris 2.x"

/*
 * Solaris curses tputs() uses a putc() with a char arg.
 * This definition overrides that in ti.c so we don't get
 * warnings during the compile.
 */
#define TI_PUTC_ARG     char

#endif /*ICI_CONF_H*/


#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following macros control the inclusion of various ICI intrinsic
 * functions. If the macro is defined the functions are NOT include
 * (hence the NO at the start of the name). By undef'ing the macro you
 * are stating that the functions should be included and compile (and
 * possibly even work) for the particular port of ICI.
 */
#undef  NOMATH          /* Trig and etc. */
#define NOTRACE         /* For debugging. */
#define NOWAITFOR       /* Requires select() or similar system primitive. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#define NOPROFILE       /* Profiler, see profile.c. */
#define NOSYSERR    /* No table of system errors */
#undef  NOSTRERR    /* Use strerror( errno ) for error strings */
#define NOSIGNALS   /* ICI level signal handling */
/*
 * This string gets compiled into the executable.
 */
#define CONFIG_STR      "BeOS Platform (r4/x86)"

#define ICI_DLL_EXT     ".im"
 /* im == ICI Module */

/*
 * ICI_DLL is defined by dynamic link ICI libraries before they include
 * any ICI includes. This allows us to declare data items as imports into
 * the DLL. Otherwise they are not visible. See fwd.h.
 */

#ifndef  ICI_CORE
#define DLI   __declspec(dllimport)
#endif

#endif /*ICI_CONF_H*/

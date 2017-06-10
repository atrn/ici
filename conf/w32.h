#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The following macros control the inclusion of various ICI intrinsic
 * functions. If the macro is defined the functions are NOT include
 * (hence the NO at the start of the name). By undef'ing the macro you
 * are stating that the functions should be included and compile (and
 * possibly even work) for the particular port of ICI.
 */
#undef  NOMATH          /* Trig and etc. */
#define NOWAITFOR       /* Requires select() or similar system primitive. */
#define NOPIPES         /* Requires popen(). */
#undef  NODIR           /* Directory reading function, dir(). */
#undef  NODLOAD         /* Dynamic loading of native machine code modules. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#undef  NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */

/*
 * ICI_CORE is defined by core language files, as opposed to builds of
 * extension libraries that are including the same include files but are
 * destined for dynamic loading against the core language at run-time.
 * This allows us to declare data items as imports into the DLL.
 * Otherwise they are not visible. See fwd.h.
 *
 * Every core language source code file defines this before including
 * any includes.
 */
#ifndef ICI_CORE
#define DLI   __declspec(dllimport) /* See DLI in fwd.h. */
#endif

#ifndef _WIN32
#define _WIN32
#endif

int ici_get_last_win32_error(void);

#define ICI_PATH_SEP    ';'
#define ICI_DIR_SEP     '\\'
#define ICI_DLL_EXT     ".dll"
/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_USE_WIN32_THREADS
/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Windows"

/*
 * Visual C++ doesn't have va_copy().  This works.
 */
#define	va_copy(d,s)	(d) = (s)

/*
 * To cope with Microsoft deprecating POSIX names...
 */

#pragma warning(disable: 4996)

#define popen           _popen
#define pclose          _pclose

#endif /*ICI_CONF_H*/

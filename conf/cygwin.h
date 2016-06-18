#ifndef ICI_CONF_H
#define ICI_CONF_H

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

#undef  NOMATH          /* Trig and etc. */
#undef  NOPASSWD        /* UNIX password file access. */
#undef  NOSTARTUPFILE   /* Parse a standard file of ICI code at init time. */
#undef  NODEBUGGING     /* Debugger interface and functions */
#define NOEVENTS        /* Event loop and associated processing. */
#undef  NOPROFILE       /* Profiler, see profile.c. */

#define ICI_USE_POSIX_THREADS

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

#define ICI_DLL_EXT     ".dll"

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Mentioned in the version string.
 */
#define CONFIG_STR      "Cygwin"

#endif /*ICI_CONF_H*/

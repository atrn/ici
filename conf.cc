#define ICI_CORE
#include "fwd.h"
#include "func.h"

namespace ici
{

extern ici_cfunc_t  ici_std_cfuncs[];
extern ici_cfunc_t  ici_save_restore_cfuncs[];
extern ici_cfunc_t  ici_re_cfuncs[];
extern ici_cfunc_t  ici_oo_cfuncs[];
extern ici_cfunc_t  ici_apl_cfuncs[];
extern ici_cfunc_t  ici_load_cfuncs[];
extern ici_cfunc_t  ici_parse_cfuncs[];

#ifndef NOEVENTS
extern ici_cfunc_t  ici_event_cfuncs[];
#endif
#ifndef NOPROFILE
extern ici_cfunc_t  ici_profile_cfuncs[];
#endif
extern ici_cfunc_t  ici_signals_cfuncs[];
#ifndef NODEBUGGING
extern ici_cfunc_t  ici_debug_cfuncs[];
#endif
extern ici_cfunc_t  ici_thread_cfuncs[];
extern ici_cfunc_t  ici_channel_cfuncs[];

ici_cfunc_t *ici_funcs[] =
{
    ici_std_cfuncs,
    ici_save_restore_cfuncs,
    ici_re_cfuncs,
    ici_oo_cfuncs,
    ici_apl_cfuncs,
    ici_load_cfuncs,
    ici_parse_cfuncs,
#ifndef NODEBUGGING
    ici_debug_cfuncs,
#endif
#ifndef NOEVENTS
    ici_event_cfuncs,
#endif
#ifndef NOPROFILE
    ici_profile_cfuncs,
#endif
    ici_signals_cfuncs,
    ici_thread_cfuncs,
    ici_channel_cfuncs,
    NULL
};

/*
 * All this does is define a string to identify the version number
 * of the interpreter. Update for new releases.
 *
 * Note test for defined(_WIN32) - MS C defines __STDC__ only when in
 * strict ANSI mode (/Za option) but this causes other problems.
 *
 */
#if defined(__STDC__) || defined(_WIN32)
/*
 * Eg: @(#)ICI 2.0.1, conf-sco.h, Apr 20 1994 11:42:12, SCO config (math win waitfor pipes )
 *
 * Note that the version number also occurs in some defines in fwd.h (ICI_*_VER)
 * and Makefile.maint.
 */
char ici_version_string[] =
    "@(#)ANICI 5.0.0, "
    CONFIG_FILE ", "
    __DATE__ " " __TIME__ ", "
    CONFIG_STR
    " ("

#ifndef NDEBUG
    "DEBUG-BUILD "
#endif
    "math "
    "waitfor "
    "system "
    "pipes "
    "dir "
    "load "
#ifndef NOSTARTUPFILE
    "startupfile "
#endif
#ifndef NODEBUGGING
    "debugging "
#endif
    "signals "

    ")";

#else /* __STDC__ */

char ici_version_string[] = "@(#)ICI 4.2.0";

#endif

} // namespace ici

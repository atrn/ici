#define ICI_CORE
#include "fwd.h"
#include "func.h"

namespace ici
{

extern cfunc  ici_std_cfuncs[];
extern cfunc  ici_save_restore_cfuncs[];
extern cfunc  ici_re_cfuncs[];
extern cfunc  ici_oo_cfuncs[];
extern cfunc  ici_apl_cfuncs[];
extern cfunc  ici_load_cfuncs[];
extern cfunc  ici_parse_cfuncs[];
extern cfunc  ici_sys_cfuncs[];
extern cfunc  ici_signals_cfuncs[];
extern cfunc  ici_thread_cfuncs[];
extern cfunc  ici_channel_cfuncs[];
extern cfunc  ici_net_cfuncs[];
#ifndef NOEVENTS
extern cfunc  ici_event_cfuncs[];
#endif
#ifndef NOPROFILE
extern cfunc  ici_profile_cfuncs[];
#endif
#ifndef NODEBUGGING
extern cfunc  ici_debug_cfuncs[];
#endif

cfunc *ici_funcs[] =
{
    ici_std_cfuncs,
    ici_save_restore_cfuncs,
    ici_re_cfuncs,
    ici_oo_cfuncs,
    ici_apl_cfuncs,
    ici_load_cfuncs,
    ici_parse_cfuncs,
    ici_signals_cfuncs,
    ici_thread_cfuncs,
    ici_sys_cfuncs,
    ici_net_cfuncs,
    ici_channel_cfuncs,
#ifndef NODEBUGGING
    ici_debug_cfuncs,
#endif
#ifndef NOEVENTS
    ici_event_cfuncs,
#endif
#ifndef NOPROFILE
    ici_profile_cfuncs,
#endif
    nullptr
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
char version_string[] =
    "@(#)ICI 5.0.0, "
    CONFIG_FILE ", "
    __DATE__ " " __TIME__ ", "
    CONFIG_STR
#ifndef NDEBUG
    " DEBUG-BUILD"
#endif
    "";

#else /* __STDC__ */

char version_string[] = "@(#)ICI 5.0.0";

#endif

} // namespace ici

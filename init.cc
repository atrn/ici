#define ICI_CORE
#include "fwd.h"
#include "func.h"
#include "buf.h"
#include "map.h"
#include "exec.h"
#include "str.h"
#include "pcre.h"
#include "archiver.h"

namespace ici
{

#define	INITIAL_ATOMSZ  (1024) /* Must be power of two. */
#define INITIAL_OBJS    (4096)

extern cfunc *ici_funcs[];

/*
 * Perform basic interpreter setup. Return non-zero on failure, usual
 * conventions.
 *
 * After calling this the scope stack has a struct for autos on it, and
 * the super of that is for statics. That struct for statics is where
 * global definitions that are likely to be visible to all code tend
 * to get set. All the intrinsic functions for example. It forms the
 * extern scope of any files parsed at the top level.
 *
 * In systems supporting threads, on exit, the global ICI mutex has been
 * acquired (with enter()). 
 *
 * This --func-- forms part of the --ici-api--.
 */
int init()
{
    extern int sys_init(objwsup *);
    extern int net_init();

    cfunc       **cfp;
    map         *scope;
    objwsup     *externs;
    exec        *x;
    int         i;
    double      pi = 3.1415926535897932;
    
    /*
     * Just make sure our universal headers are really the size we
     * hope they are. Nothing actually assumes this. But it would
     * represent a significant inefficiency if they were padded.
     */
    assert(sizeof (object) == 4);

    /*
     * The following assertion is only valid on some architectures.
     * In particular it is wrong for systems that use 64-bit aligned
     * pointers.
     *
    assert(offsetof(objwsup, o_super) == 4);
     */

#   ifndef NDEBUG
    {
        char            v[80];
        /*
         * Check that the #defines of version number are in sync with our version
         * string from conf.c.
         */
        sprintf(v, "@(#)ANICI %d.%d.%d", major_version, minor_version, release_number);
        assert(strncmp(v, version_string, strlen(v)) == 0);
    }
#   endif

    if (chkbuf(1024))
    {
        return 1;
    }
    if ((atoms = (object **)ici_nalloc(INITIAL_ATOMSZ * sizeof (object *))) == NULL)
    {
        return 1;
    }
    atomsz = INITIAL_ATOMSZ;
    memset((char *)atoms, 0, atomsz * sizeof (object *));
    if ((objs = (object **)ici_nalloc(INITIAL_OBJS * sizeof (object *))) == NULL)
    {
        return 1;
    }
    memset((char *)objs, 0, INITIAL_OBJS * sizeof (object *));
    objs_limit = objs + INITIAL_OBJS;
    objs_top = objs;
    for (i = 0; i < (int)nels(small_ints); ++i)
    {
        if ((small_ints[i] = new_int(i)) == NULL)
        {
            return -1;
        }
    }
    o_zero = small_ints[0];
    o_one = small_ints[1];
    if (init_sstrings())
    {
        return 1;
    }
    pcre_free = ici_free;
    pcre_malloc = (void *(*)(size_t))ici_alloc;
    if ((scope = new_map()) == NULL)
    {
        return 1;
    }
    if ((scope->o_super = externs = objwsupof(new_map())) == NULL)
    {
        return 1;
    }
    externs->decref();
    if ((x = new_exec()) == NULL)
    {
        return 1;
    }
    enter(x);
    rego(&os);
    rego(&xs);
    rego(&vs);
    if (engine_stack_check())
    {
        return 1;
    }
    vs.push(scope, owned);
    if (init_path(externs))
    {
        return 1;
    }
    for (cfp = ici_funcs; *cfp != NULL; ++cfp)
    {
        if (assign_cfuncs(scope->o_super, *cfp))
        {
            return 1;
        }
    }
    if (sys_init(scope->o_super))
    {
        return 1;
    }
    if (net_init())
    {
        return 1;
    }
    if (archive_init())
    {
        return 1;
    };
    signals_init();

    if
    (
        set_val(externs, SS(_stdin),  'u', stdin)
        ||
        set_val(externs, SS(_stdout), 'u', stdout)
        ||
        set_val(externs, SS(_stderr), 'u', stderr)
        ||
        set_val(externs, SS(pi), 'f', &pi)
    )
    {
        return 1;
    }

#ifndef NOSTARTUPFILE
    if (call(SS(load), "o", SS(core)))
    {
        return 1;
    }
#endif

    return 0;
}

/*
 * Check that the seperately compiled module that calls this function has been
 * compiled against a compatible versions of the ICI core that is now trying
 * to load it. An external module should call this like:
 *
 *     if (ici::check_interface(version_number, back_compat_version, "myname"))
 *         return NULL;
 *
 * As soon as it can on load.  ICI_VER and ICI_BACK_COMPAT_VER come from ici.h
 * at the time that module was compiled.  This functions compares the values
 * passed from the external modules with the values the core was compiled
 * with, and fails (usual conventions) if there is any incompatibility.
 *
 * This --func-- forms part of the --ici-api--.
 */
int check_interface(unsigned long mver, unsigned long bver, char const *name)
{
    if (version_number < mver)
    {
        /*
         * Ooh, I'm an old ICI being called by a newer module. Does the module
         * think I'm recent enought?
         */
        if (version_number >= bver)
        {
            return 0;
        }
    }
    else
    {
        /*
         * I'm a relatively up-to-date ICI, but is that module new enough
         * for me?
         */
        if (mver >= back_compat_version)
        {
            return 0;
        }
    }
    return set_error
    (
        "%s module was built for ANICI %d.%d.%d, which is incompatible with this version %d.%d.%d",
        name,
        (int)(mver >> 24),
        (int)(mver >> 16) & 0xFF,
        (int)(mver & 0xFFFF),
        major_version,
        minor_version,
        release_number);

}

} // namespace ici

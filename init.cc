#define ICI_CORE
#include "archiver.h"
#include "buf.h"
#include "exec.h"
#include "func.h"
#include "fwd.h"
#include "map.h"
#include "pcre.h"
#include "ref.h"
#include "str.h"

#if defined(_WIN32)
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace ici
{

constexpr size_t INITIAL_ATOMSZ = 1024; // Must be power of two
constexpr size_t INITIAL_OBJS = 4096;

extern cfunc *ici_funcs[];
static int    mapici_init(objwsup *);
extern int    sys_init(objwsup *);
extern int    net_init(objwsup *);

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
    static bool init_done = false;
    if (init_done)
    {
        return 0;
    }

    cfunc  **cfp;
    map     *scope;
    objwsup *externs;
    exec    *x;
    int      i;
    double   pi = 3.14159265358979323848;

    /*
     * Just make sure our universal headers are really the size we
     * hope they are. Nothing actually assumes this. But it would
     * represent a significant inefficiency if they were padded.
     */
    assert(sizeof(object) == 4);

    /*
     * The following assertion is only valid on some architectures.
     * In particular it is wrong for systems that use 64-bit aligned
     * pointers.
     *
    assert(offsetof(objwsup, o_super) == 4);
     */

#ifndef NDEBUG
    {
        char v[80];
        /*
         * Check that the #defines of version number are in sync with our version
         * string from conf.c.
         */
        sprintf(v, "@(#)ICI %d.%d.%d", major_version, minor_version, release_number);
        assert(strncmp(v, version_string, strlen(v)) == 0);
    }
#endif

    if (chkbuf(1024))
    {
        return 1;
    }
    if ((atoms = (object **)ici_nalloc(INITIAL_ATOMSZ * sizeof(object *))) == nullptr)
    {
        return 1;
    }
    atomsz = INITIAL_ATOMSZ;
    memset((char *)atoms, 0, atomsz * sizeof(object *));
    if ((objs = (object **)ici_nalloc(INITIAL_OBJS * sizeof(object *))) == nullptr)
    {
        return 1;
    }
    memset((char *)objs, 0, INITIAL_OBJS * sizeof(object *));
    objs_limit = objs + INITIAL_OBJS;
    objs_top = objs;
    for (i = 0; i < (int)nels(small_ints); ++i)
    {
        if ((small_ints[i] = new_int(i)) == nullptr)
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
    if ((scope = new_map()) == nullptr)
    {
        return 1;
    }
    if ((scope->o_super = externs = objwsupof(new_map())) == nullptr)
    {
        return 1;
    }
    decref(externs);
    if ((x = new_exec()) == nullptr)
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
    vs.push(scope, with_decref);
    if (mapici_init(externs))
    {
        return 1;
    }
    for (cfp = ici_funcs; *cfp != nullptr; ++cfp)
    {
        if (assign_cfuncs(externs, *cfp))
        {
            return 1;
        }
    }
    if (sys_init(externs))
    {
        return 1;
    }
    if (net_init(externs))
    {
        return 1;
    }
    if (archive_init())
    {
        return 1;
    };
    init_signals();
    init_exec();
    if (set_val(externs, SS(_stdin), 'u', stdin) || set_val(externs, SS(_stdout), 'u', stdout) ||
        set_val(externs, SS(_stderr), 'u', stderr) || set_val(externs, SS(pi), 'f', &pi))
    {
        return 1;
    }

#ifndef NOSTARTUPFILE
    if (call(SS(load), "o", SS(core)))
    {
        return 1;
    }
#endif

    init_done = true;
    return 0;
}

/*
 * Check that the seperately compiled module that calls this function has been
 * compiled against a compatible versions of the ICI core that is now trying
 * to load it. An external module should call this like:
 *
 *     if (ici::check_interface(version_number, back_compat_version, "myname"))
 *         return nullptr;
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
    return set_error("%s module was built for ICI %d.%d.%d, which is incompatible with this version %d.%d.%d", name,
                     (int)(mver >> 24), (int)(mver >> 16) & 0xFF, (int)(mver & 0xFFFF), major_version, minor_version,
                     release_number);
}

static int push_path_elements(array *a, const char *path); /* Forward. */
#define PUSH(A, B)                              \
    do                                          \
    {                                           \
        if (push_path_elements((A), (B)))       \
        {                                       \
            return 1;                           \
        }                                       \
    }                                           \
    while (0)

/*
 * Push one or more file names from path, seperated by the local system
 * seperator character (eg. : or ;), onto a. Usual error conventions.
 */
static int push_path_elements(array *a, const char *path)
{
    const char *p;
    const char *q;
    str        *s;
    object    **e;

    for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
    {
        if ((q = strchr(p, ICI_PATH_SEP)) == nullptr)
        {
            q = p + strlen(p);
        }
        if ((s = new_str(p, q - p)) == nullptr)
        {
            return 1;
        }
        /*
         * Don't add duplicates...
         */
        for (e = a->a_base; e < a->a_top; ++e)
        {
            if (*e == s)
            {
                goto skip;
            }
        }
        /*
         * Don't add inaccessable dirs...
         */
        if (access(s->s_chars, 0) != 0)
        {
            goto skip;
        }
        if (a->push_checked(s))
        {
            decref(s);
            return 1;
        }
skip:
        decref(s);
    }
    return 0;
}

#if defined(_WIN32)
/*
 * Push path elements specific to Windows onto the array a (which is the ICI
 * path array used for finding dynamically loaded modules and stuff). These
 * are in addition to the ICIPATH environment variable. We try to mimic
 * the search behaviour of LoadLibrary() (that being the Windows thing to
 * do).
 */
static int push_os_path_elements(array *a)
{
    char  fname[MAX_PATH];
    char *p;

    if (GetModuleFileName(nullptr, fname, sizeof fname - 10) > 0)
    {
        if ((p = strrchr(fname, ICI_DIR_SEP)) != nullptr)
        {
            *p = '\0';
        }
        PUSH(a, fname);
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        PUSH(a, fname);
    }
    PUSH(a, ".");
    if (GetSystemDirectory(fname, sizeof fname - 10) > 0)
    {
        PUSH(a, fname);
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        PUSH(a, fname);
    }
    if (GetWindowsDirectory(fname, sizeof fname - 10) > 0)
    {
        PUSH(a, fname);
        p = fname + strlen(fname);
        *p++ = ICI_DIR_SEP;
        strcpy(p, "ici");
        PUSH(a, fname);
    }
    if ((p = getenv("PATH")) != nullptr)
    {
        PUSH(a, p);
    }
    return 0;
}
#else
/*
 * Assume a UNIX-like sytem.
 *
 * Push path elements specific to UNIX-like systems onto the array a (which
 * is the ICI path array used for finding dynamically loaded modules and
 * stuff). These are in addition to the ICIPATH environment variable.
 */
static int push_os_path_elements(array *a)
{
    char *p;
    char *q;
    char *path;
    char  fname[FILENAME_MAX];

    PUSH(a, "/usr/lib/ici:/usr/local/lib/ici:/opt/lib/ici:/opt/ici/lib/ici:.");
    if ((path = getenv("PATH")) != nullptr)
    {
        for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
        {
            if ((q = strchr(p, ':')) == nullptr)
            {
                q = p + strlen(p);
            }
            if (q - 4 < p || memcmp(q - 4, "/bin", 4) != 0 || q - p >= FILENAME_MAX - 10)
            {
                continue;
            }
            memcpy(fname, p, (q - p) - 4);
            strcpy(fname + (q - p) - 4, "/lib/ici");
            PUSH(a, fname);
        }
    }
#ifdef ICI_CONFIG_PREFIX
    /*
     * Put a configuration defined location on, if there is one..
     */
    PUSH(a, ICI_CONFIG_PREFIX "/lib/ici");
#endif
    return 0;
}
#endif /* End of selection of which push_os_path_elements() to use */

/*
 * Set the path variable in externs to be an array of all the directories
 * that should be searched in for ICI extension modules and stuff.
 */
ref<array> init_path()
{
    ref<array> a = new_array();
    if (a)
    {
        if (char *path = getenv("ICIPATH"))
        {
            if (push_path_elements(a, path))
            {
                return nullptr;
            }
        }
        push_os_path_elements(a);
    }
    return a;
}

static int mapici_init(objwsup *externs)
{
    ref<str> ver = new_str_nul_term(version_string);
    if (!ver)
    {
        return 1;
    }
    auto path = init_path();
    if (!path)
    {
        return 1;
    }
    ref<map> mapici = new_map();
    if (!mapici)
    {
        return 1;
    }
    if (mapici->assign(SS(version), ver))
    {
        return 1;
    }
    if (mapici->assign(SS(path), path))
    {
        return 1;
    }
    if (externs->assign(SS(_ici), mapici))
    {
        return 1;
    }
    return 0;
}

} // namespace ici

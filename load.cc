#define ICI_CORE
#include "exec.h"
#include "str.h"
#include "map.h"
#include "file.h"
#include "buf.h"
#include "func.h"

namespace ici
{

static                  int push_path_elements(array *a, const char *path); /* Forward. */
#define PUSH(A, B)      if (push_path_elements((A), (B))) return 1

/*
 * If we're supporting loading of native code modules we need Unix-style
 * dlopen/dlsym/dlclose routines.  Pick a version according to the platform.
 */

# if defined(_WIN32)
#  include "load-w32.h"
#  include <io.h>
# else /* Unix */
#  include <sys/types.h>
#  include <unistd.h>
#  include <dlfcn.h>

typedef void    *dll_t;
#define valid_dll(dll)  ((dll) != NULL)

# endif /* _WIN32 */

#ifndef NODLOAD
# ifndef RTLD_NOW
#  define RTLD_NOW 1
# endif
# ifndef RTLD_GLOBAL
#  define RTLD_GLOBAL 0
# endif
#endif

static const char   ici_prefix[] = "ici-";

/*
 * Find and return the outer-most writeable struct in the current scope.
 * This is typically the struct that contains all the ICI core language
 * functions. This is the place we lodge new dynamically loaded modules
 * and is also the typical parent for top level classes made by dynamically
 * loaded modules. Usual error conventions.
 */
objwsup *outermost_writeable()
{
    objwsup       *outer;
    objwsup       *ows;

    outer = NULL;
    for (ows = objwsupof(vs.a_top[-1]); ows != NULL; ows = ows->o_super)
    {
        if (ows->isatom())
            continue;
        if (!ismap(ows))
            continue;
        outer = ows;
    }
    if (outer == NULL)
        set_error("no writeable structs in current scope");
    return outer;
}

/*
 * any = load(string)
 *
 * Function form of auto-loading. String is the name of a file to be
 * loaded stripped of its ``ici'' prefix and ``.ici'' suffix, i.e., it is
 * the same name that would be used in the ici source to refer to the
 * module. The result is the object that would have been assigned to that
 * name in the scope if the module had been auto-loaded, i.e., for an
 * ici module it is the reference to the extern scope of the module, for
 * a native code module it the object returned by that module's init.
 * function.
 */
static int
f_load(...)
{
    str        *name;
    object     *result;
    char        fname[FILENAME_MAX];
    char        entry_symbol[64];
    map *statics;
    map *autos;
    map *externs;
    objwsup    *outer;
    file       *file;
    FILE       *stream;

    externs = NULL;
    statics = NULL;
    autos = NULL;
    file = NULL;
    result = NULL;

    /*
     * This is here to stop a compiler warning, complaining that entry_symbol
     * is never used (but we know it is)
     */
    entry_symbol[0] = 0;

    if (typecheck("o", &name))
        return 1;
    if (!isstring(name))
        return argerror(0);
    /*
     * Find the outer-most writeable scope. This is where the new name
     * will be defined should it be loadable as a library.
     */
    if ((outer = outermost_writeable()) == NULL)
        return 0;

    /*
     * Simplify a lot of checks by simply ignoring huge module names.
     */
    if (name->s_nchars > (int)(sizeof entry_symbol - 20))
        return 0;

    /*
     * First of all we consider the case of a dynamically loaded native
     * machine code.
     */
    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);

    if (find_on_path(fname, ICI_DLL_EXT))
    {
        dll_t           lib;
        object       *(*library_init)();
        object       *o;

        /*
         * We have a file .../iciX.EXT. Attempt to dynamically load it.
         */
        lib = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
        if (!valid_dll(lib))
        {
            const char  *err;

            if ((err = (char *)dlerror()) == NULL)
                err = "dynamic link error";
            set_error("failed to load \"%s\", %s", fname, err);
            return -1;
        }
#ifdef NEED_UNDERSCORE_ON_SYMBOLS
        sprintf(entry_symbol, "_ici_%s_init", name->s_chars);
#else
        sprintf(entry_symbol, "ici_%s_init", name->s_chars);
#endif
        library_init = (object *(*)())dlsym(lib, entry_symbol);
        if (library_init == NULL)
        {
#ifndef SUNOS5 /* Doing the dlclose results in a crash under Solaris - why? */
            dlclose(lib);
#endif
            if (chkbuf(strlen(entry_symbol) + strlen(fname)))
                set_error("failed to find library entry point");
            else
            {
                set_error("failed to find the entry symbol %s in \"%s\"",
                    entry_symbol, fname);
            }
            return -1;
        }
        if ((o = (*library_init)()) == NULL)
        {
            dlclose(lib);
            return -1;
        }
        result = o;
        if (ici_assign(outer, name, o))
            goto fail;
        if (!ismap(o))
            return ret_with_decref(o);
        externs = mapof(o);
    }

    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);
    if (find_on_path(fname, ".ici"))
    {
        str       *fn;
        /*
         * We have a file .../iciX.ici, open the file, make new statics
         * and autos, assign the statics into the extern scope under
         * the given name, then parse the new module.
         */
        if ((stream = fopen(fname, "r")) == NULL)
            return get_last_errno("open", fname);
        if ((fn = new_str_nul_term(fname)) == NULL)
        {
            fclose(stream);
            goto fail;
        }
        file = new_file((char *)stream, stdio_ftype, fn, NULL);
        fn->decref();
        if (file == NULL)
        {
            fclose(stream);
            goto fail;
        }
        if ((autos = new_map()) == NULL)
        {
            goto fail;
        }
        if ((statics = new_map()) == NULL)
        {
            goto fail;
        }
        autos->o_super = objwsupof(statics);
        statics->decref();
        if (externs == NULL)
        {
            if ((externs = new_map()) == NULL)
            {
                goto fail;
            }
            result = externs;
            if (ici_assign(outer, name, externs))
            {
                goto fail;
            }
        }
        statics->o_super = objwsupof(externs);
        externs->o_super = outer;
        if (parse_file(file, objwsupof(autos)) < 0)
        {
            goto fail;
        }
        close_file(file);
        autos->decref();
        file->decref();
    }
    if (result == NULL)
    {
        set_error("\"%s\" undefined and could not find %s%s%s or %s%s.ici ",
            name->s_chars,
            ici_prefix,
            name->s_chars,
            ICI_DLL_EXT,
            ici_prefix,
            name->s_chars);
        goto fail;
    }
    return ret_with_decref(result);

fail:
    if (file != NULL)
        file->decref();
    if (autos != NULL)
        autos->decref();
    if (result != NULL)
        result->decref();
    return 1;
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
    char                fname[MAX_PATH];
    char                *p;

    if (GetModuleFileName(NULL, fname, sizeof fname - 10) > 0)
    {
        if ((p = strrchr(fname, ICI_DIR_SEP)) != NULL)
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
    if ((p = getenv("PATH")) != NULL)
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
    char                *p;
    char                *q;
    char                *path;
    char                fname[FILENAME_MAX];

    PUSH(a, "/usr/lib/ici:/usr/local/lib/ici:/opt/ici:/opt/lib/ici:.");
    if ((path = getenv("PATH")) != NULL)
    {
        for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
        {
            if ((q = strchr(p, ':')) == NULL)
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
#   ifdef ICI_CONFIG_PREFIX
    /*
     * Put a configuration defined location on, if there is one..
     */
    PUSH(a, ICI_CONFIG_PREFIX "/lib/ici");
#   endif
    return 0;
}
#endif /* End of selection of which push_os_path_elements() to use */

/*
 * Push one or more file names from path, seperated by the local system
 * seperator character (eg. : or ;), onto a. Usual error conventions.
 */
static int push_path_elements(array *a, const char *path)
{
    const char  *p;
    const char  *q;
    str         *s;
    object     **e;

    for (p = path; *p != '\0'; p = *q == '\0' ? q : q + 1)
    {
        if ((q = strchr(p, ICI_PATH_SEP)) == NULL)
        {
            q = p + strlen(p);
        }
        if ((s = new_str(p, q - p)) == NULL)
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
        if (a->push_check())
        {
            s->decref();
            return 1;
        }
        a->push(s);
    skip:
        s->decref();
    }
    return 0;
}

/*
 * Set the path variable in externs to be an array of all the directories
 * that should be searched in for ICI extension modules and stuff.
 */
int init_path(objwsup *externs)
{
    array *a;
    int    r;
    char  *path;

    if ((a = new_array()) == NULL)
    {
        return 1;
    }
    r = ici_assign_base(externs, SS(path), a);
    a->decref();
    if (r)
    {
        return 1;
    }
    if ((path = getenv("ICIPATH")) != NULL)
    {
        PUSH(a, path);
    }
    push_os_path_elements(a);
    return 0;
}

ICI_DEFINE_CFUNCS(load)
{
    ICI_DEFINE_CFUNC(load, f_load),
    ICI_CFUNCS_END()
};

} // namespace ici

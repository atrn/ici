#define ICI_CORE
#include "buf.h"
#include "cfunc.h"
#include "exec.h"
#include "file.h"
#include "func.h"
#include "map.h"
#include "str.h"

namespace ici
{

/*
 * If we're supporting loading of native code modules we need Unix-style
 * dlopen/dlsym/dlclose routines.  Pick a version according to the platform.
 */

#if defined(_WIN32)
#include "load-w32.h"
#include <io.h>
#else /* Unix */
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>

typedef void *dll_t;
#define valid_dll(dll) ((dll) != nullptr)

#endif /* _WIN32 */

#ifndef NODLOAD
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#endif

static const char ici_prefix[] = "ici-";

/*
 * Find and return the outer-most writeable struct in the current scope.
 * This is typically the struct that contains all the ICI core language
 * functions. This is the place we lodge new dynamically loaded modules
 * and is also the typical parent for top level classes made by dynamically
 * loaded modules. Usual error conventions.
 */
objwsup *outermost_writeable()
{
    objwsup *outer;
    objwsup *ows;

    outer = nullptr;
    for (ows = objwsupof(vs.a_top[-1]); ows != nullptr; ows = ows->o_super)
    {
        if (ows->isatom())
        {
            continue;
        }
        if (!ismap(ows))
        {
            continue;
        }
        outer = ows;
    }
    if (outer == nullptr)
    {
        set_error("no writeable structs in current scope");
    }
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
static int f_load(...)
{
    str     *name;
    object  *result;
    char     fname[FILENAME_MAX];
    char     entry_symbol[64];
    map     *statics;
    map     *autos;
    map     *externs;
    objwsup *outer;
    file    *file;
    FILE    *stream;

    externs = nullptr;
    statics = nullptr;
    autos = nullptr;
    file = nullptr;
    result = nullptr;

    /*
     * This is here to stop a compiler warning, complaining that entry_symbol
     * is never used (but we know it is)
     */
    entry_symbol[0] = 0;

    if (typecheck("o", &name))
    {
        return 1;
    }
    if (!isstring(name))
    {
        return argerror(0);
    }
    /*
     * Find the outer-most writeable scope. This is where the new name
     * will be defined should it be loadable as a library.
     */
    if ((outer = outermost_writeable()) == nullptr)
    {
        return 0;
    }

    /*
     * Simplify a lot of checks by simply ignoring huge module names.
     */
    if (name->s_nchars > (int)(sizeof entry_symbol - 20))
    {
        return 0;
    }

    /*
     * First of all we consider the case of a dynamically loaded native
     * machine code.
     */
    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);

    if (find_on_path(fname, ICI_DLL_EXT))
    {
        dll_t lib;
        object *(*library_init)();
        object *o;

        /*
         * We have a file .../iciX.EXT. Attempt to dynamically load it.
         */
        lib = dlopen(fname, RTLD_NOW | RTLD_GLOBAL);
        if (!valid_dll(lib))
        {
            const char *err;

            if ((err = (char *)dlerror()) == nullptr)
            {
                err = "dynamic link error";
            }
            set_error("failed to load \"%s\", %s", fname, err);
            return -1;
        }
#ifdef NEED_UNDERSCORE_ON_SYMBOLS
        sprintf(entry_symbol, "_ici_%s_init", name->s_chars);
#else
        sprintf(entry_symbol, "ici_%s_init", name->s_chars);
#endif
        library_init = (object * (*)()) dlsym(lib, entry_symbol);
        if (library_init == nullptr)
        {
#ifndef SUNOS5 /* Doing the dlclose results in a crash under Solaris - why? */
            dlclose(lib);
#endif
            if (chkbuf(strlen(entry_symbol) + strlen(fname)))
            {
                set_error("failed to find library entry point");
            }
            else
            {
                set_error("failed to find the entry symbol %s in \"%s\"", entry_symbol, fname);
            }
            return -1;
        }
        if ((o = (*library_init)()) == nullptr)
        {
            dlclose(lib);
            return -1;
        }
        result = o;
        if (ici_assign(outer, name, o))
        {
            goto fail;
        }
        if (!ismap(o))
        {
            return ret_with_decref(o);
        }
        externs = mapof(o);
    }

    strcpy(fname, ici_prefix);
    strcat(fname, name->s_chars);

    if (find_on_path(fname, ".ici"))
    {
        str *fn;
        /*
         * We have a file .../iciX.ici, open the file, make new statics
         * and autos, assign the statics into the extern scope under
         * the given name, then parse the new module.
         */
        if ((stream = fopen(fname, "r")) == nullptr)
        {
            return get_last_errno("open", fname);
        }
        if ((fn = new_str_nul_term(fname)) == nullptr)
        {
            fclose(stream);
            goto fail;
        }
        file = new_file((char *)stream, stdio_ftype, fn, nullptr);
        decref(fn);
        if (file == nullptr)
        {
            fclose(stream);
            goto fail;
        }
        if ((autos = new_map()) == nullptr)
        {
            goto fail;
        }
        if ((statics = new_map()) == nullptr)
        {
            goto fail;
        }
        autos->o_super = objwsupof(statics);
        decref(statics);
        if (externs == nullptr)
        {
            if ((externs = new_map()) == nullptr)
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
        decref(autos);
        decref(file);
    }

    if (result == nullptr)
    {
        set_error("\"%s\" undefined ", name->s_chars);
        goto fail;
    }

    return ret_with_decref(result);

fail:
    if (file != nullptr)
    {
        decref(file);
    }
    if (autos != nullptr)
    {
        decref(autos);
    }
    if (result != nullptr)
    {
        decref(result);
    }
    return 1;
}

ICI_DEFINE_CFUNCS(load){ICI_DEFINE_CFUNC(load, f_load), ICI_CFUNCS_END()};

} // namespace ici

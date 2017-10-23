#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>
#include "load.h"
#include "dllmanager.h"
#include "dll.h"
#include "dllfunc.h"

int                     dllmanager_tcode;

/*
 * Call dynamic libs directly
 * 
 * The ICI 'dll' module allows the calling of pre-existing functions
 * in dynamic loaded libraries directly from ICI without any
 * explicitly compiled linkage. Calls have the form:
 *
 *  dll.libname.funcname(args...)
 *
 * Where 'libname' is used to determine a dynamically loadable library
 * using the ususal conventions of the local system. (For example, it
 * might find 'libname.dll' on Windows, or 'liblibname.so' on many
 * UNIX-like systems. The usual system search paths will be used.)
 *
 * The first time a particular name (such as 'libname' in this example)
 * is used to index the 'dll' object, it is resolved to a library which
 * is loaded into the current processes address space. A special object
 * that refers to this library is then assigned to that field of the
 * 'dll' object. Thus subsequent references to the same library will
 * return the reference to the same loaded library.
 *
 * Function names are resolved in a similar fashion. That is, on first
 * index the name is looked up in the library and a special internal
 * linkage function created. This is assigned to that field of the
 * loaded library object. Thus subsequent references to the same function
 * name reference the created linkage function.
 *
 * Of course actual calls to the function must have arguments and
 * return values translated between ICI values and the native forms.
 * By default, the 'dll' module will use a set of standard conventions
 * for such translatations. However for cases where the defaults are
 * not sufficient, a mechanisn for pre-declaring the calling conventions
 * of the function exists.
 *
 * This --intro-- and --synopsis-- are part of --ici-dll-- documentation.
 */
 
/*
 * Make a new dllmanager object. The dllmanager object holds a shadow struct
 * that is used to hold, by name, any dlls that have been loaded.
 *
 * Typically only one of these is ever created when the first mention of
 * 'dll' is made in a ICI program.
 */
static dllmanager_t *
new_dllmanager(void)
{
    dllmanager_t   *dllm;

    if ((dllm = ici_talloc(dllmanager_t)) == NULL)
        return NULL;
    if ((dllm->dllm_struct = ici_struct_new()) == NULL)
    {
        ici_tfree(dllm, dllmanager_t);
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(dllm, dllmanager_tcode, 0, 1, 0);
    ici_rego(ici_objof(dllm));
    ici_decref(dllm->dllm_struct);
    return dllm;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_dllmanager(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof(dllmanager_t) + ici_mark(dllmanagerof(o)->dllm_struct);
}

static void
free_dllmanager(ici_obj_t *o)
{
    ici_tfree(o, dllmanager_t);
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * We just pass this through to our backing struct.
 */
static int
assign_dllmanager(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    return ici_assign(dllmanagerof(o)->dllm_struct, k, v);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 *
 * If the key already exists in our backing struct, we just return
 * its value. Otherwise the key is interpreted as a dll name, the dll
 * is loaded (if possible) and returned, with the side effect of assigning
 * it to our backing struct as well.
 */
static ici_obj_t *
fetch_dllmanager(ici_obj_t *o, ici_obj_t *k)
{
    /*char                *path;*/
    char                fname[FILENAME_MAX];
    dll_t               lib;
    int                 gotlib;
    ici_obj_t            *v;

    gotlib = 0;
    if ((v = ici_fetch(dllmanagerof(o)->dllm_struct, k)) == NULL)
        return NULL;
    if (v != ici_null)
        return v;
    if (!ici_isstring(k) || ici_stringof(k)->s_nchars > sizeof fname - 1)
        return ici_fetch_fail(o, k);

    /*
     * First try to load it directly. (Windows)
     */
    strncpy(fname, ici_stringof(k)->s_chars, sizeof fname);
    fname[sizeof fname - 1] = '\0';
    lib = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
    if (!valid_dll(lib))
    {
#       ifndef  _WIN32
        if (strchr(fname, '/') == 0 && ici_stringof(k)->s_nchars < FILENAME_MAX - 20)
        {
            sprintf(fname, "lib%s.%s", ici_stringof(k)->s_chars, ICI_DLL_EXT);
            lib = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
        }
        else
#       endif
        {
            char        *err;
            char const  fmt[] = "failed to load %s, %s";

            if ((err = (char *)dlerror()) == NULL)
                err = "dynamic link error";
            if (ici_chkbuf(strlen(fmt) + strlen(fname) + strlen(err) + 1))
                strcpy(ici_buf, err);
            else
                sprintf(ici_buf, fmt, fname, err);
            ici_error = ici_buf;
            return NULL;
        }
    }
    gotlib = 1;
    if ((v = ici_objof(new_dll(lib))) == NULL)
        goto fail;
    if (ici_assign(dllmanagerof(o)->dllm_struct, k, v))
    {
        ici_decref(v);
        goto fail;
    }
    ici_decref(v);
    return v;

fail:
#ifndef SUNOS5 /* Doing the dlclose results in a crash under Solaris - why? */
    if (gotlib)
        dlclose(lib);
#endif
    return NULL;
}

ici_type_t dllmanager_type =
{
    mark_dllmanager,
    free_dllmanager,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    assign_dllmanager,
    fetch_dllmanager,
    "dllmanager"
};

ici_obj_t *
ici_dll_library_init(void)
{
    dllmanager_t        *dllm;

    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "dll"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((dllmanager_tcode = ici_register_type(&dllmanager_type)) == 0)
        return NULL;
    if ((dll_tcode = ici_register_type(&dll_type)) == 0)
        return NULL;
    if ((dllfunc_tcode = ici_register_type(&dllfunc_type)) == 0)
        return NULL;
    if ((dllm = new_dllmanager()) == NULL)
        return NULL;
    return ici_objof(dllm);
}

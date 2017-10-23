#define ICI_NO_OLD_NAMES

#include <ici.h>
#include "icistr.h"
#include "load.h"
#include "dllmanager.h"
#include "dll.h"
#include "dllfunc.h"

/*
 * dll.xxx:DECL(func-name, ...)
 *
 * 'DECL' allows the explicit declaration of functions that, for some
 * reason, can't be relied on to deduce their interface from the actual
 * paraeters. For example, returning a C style nul terminated string
 * rather than the default 32 bit integer.
 *
 * This --topic-- forms part of the --ici-dll-- documentation.
 */
static int
dll_decl(ici_obj_t *o)
{
    dllib_t             *dll;
    dllfunc_t           *df;
    ici_obj_t           *a;
    int                 i;
    char                n1[30];
    char                n2[30];

    if (o == NULL)
    {
        sprintf(ici_buf, "attempt to call method %s as a function",
            ici_objname(n1, ici_os.a_top[-1]));
        ici_error = ici_buf;
        return 1;
    }
    if (!isdll(o))
    {
        sprintf(ici_buf, "attempt to apply method %s to %s",
            ici_objname(n1, ici_os.a_top[-1]),
            ici_objname(n2, o));
        ici_error = ici_buf;
        return 1;
    }
    dll = dllof(o);
    if (!ici_isstring(ICI_ARG(0)))
        return ici_argerror(0);
    if ((df = new_dllfunc(dll, ici_stringof(ICI_ARG(0)), NULL)) == NULL)
        return 1;
    if (ici_assign(dll->dll_struct, ICI_ARG(0), ici_objof(df)))
    {
        ici_decref(df);
        return 1;
    }
    ici_decref(df);
    for (i = 1; i < ICI_NARGS(); ++i)
    {
        a = ICI_ARG(i);
        if (a == ICISO(stdcall) || a == ICISO(winapi))
        {
            df->df_calltype = CT_STDCALL;
        }
        else if (a == ICISO(fastcall))
        {
            df->df_calltype = CT_FASTCALL;
        }
        else if (a == ICISO(Xcdecl))
        {
            df->df_calltype = CT_CDECL;
        }
        else
        {
            sprintf(ici_buf, "%s is not a known declaration modifier",
                ici_objname(n1, ICI_ARG(i)));
            ici_error = ici_buf;
            return 1;
        }
    }
    return ici_ret_no_decref(ici_objof(df));
}

static ici_cfunc_t      dll_cfuncs[] =
{
    {ICI_CF_OBJ,    "DECL",     dll_decl},
    {ICI_CF_OBJ,    NULL,       NULL}
};


int               dll_tcode;

/*
 * Make a new dllib_t (aka dll) object that holds a reference to the loaded
 * library lib. The dll object has a shadow struct that is used to hold
 * functions, by name, as they are referenced.
 *
 * This object can be collected, but doing so does not cause the lib to be
 * unloaded or any such thing. This is because of some problems as explained
 * in dllfunc.c.
 */
dllib_t *
new_dll(dll_t lib)
{
    dllib_t             *dll;

    if ((dll = ici_talloc(dllib_t)) == NULL)
        return NULL;
    if ((dll->dll_struct = ici_struct_new()) == NULL)
    {
        ici_tfree(dll, dllib_t);
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(dll, dll_tcode, 0, 1, 0);
    dll->dll_lib = lib;
    ici_rego(ici_objof(dll));
    ici_decref(dll->dll_struct);
    if (ici_assign_cfuncs(ici_objwsupof(dll->dll_struct), dll_cfuncs))
    {
        ici_decref(dll);
        return NULL;
    }
    return dll;
}

void
free_dll(ici_obj_t *o)
{
    dlclose(dllof(o)->dll_lib);
    ici_tfree(o, dllib_t);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_dll(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof(dllib_t) + ici_mark(dllof(o)->dll_struct);
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * We just pass this through to our backing struct.
 */
static int
assign_dll(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    return ici_assign(dllof(o)->dll_struct, k, v);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 *
 * If the key already exists in our backing struct, we just return
 * its value. Otherwise the key is interpreted as a function name in the dll.
 * It is found (if possible) and returned, with the side effect of assigning
 * it to our backing struct as well.
 */
static ici_obj_t *
fetch_dll(ici_obj_t *o, ici_obj_t *k)
{
    ici_obj_t            *v;

    if ((v = ici_fetch(dllof(o)->dll_struct, k)) == NULL)
        return NULL;
    if (v != ici_null)
        return v;
    if (!ici_isstring(k))
        return ici_fetch_fail(o, k);
    if ((v = ici_objof(new_dllfunc(dllof(o), ici_stringof(k), NULL))) == NULL)
        return NULL;
    if (ici_assign(dllof(o)->dll_struct, k, v))
    {
        ici_decref(v);
        return NULL;
    }
    ici_decref(v);
    return v;
}

ici_type_t dll_type =
{
    mark_dll,
    free_dll,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    assign_dll,
    fetch_dll,
    "dll"
};

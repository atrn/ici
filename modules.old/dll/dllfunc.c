#include <ici.h>
#include "icistr.h"
#include "load.h"
#include "dllmanager.h"
#include "dll.h"
#include "dllfunc.h"
#include <assert.h>

int                     dllfunc_tcode;

#ifdef _WIN32
#define alloca _alloca
#endif

/*
 * Call the function at addr, which has the function style given by ct,
 * with the nargs 32 bit words of arguments stored in args and return
 * the 32 bit word the function returned.
 */
static long
call_long(calltype_t ct, void *addr, int nargs, long *av)
{
    /*
     * We used to have code here that used assembler to construct the
     * stack as necessary for the number of args and the calling
     * convention. But, of course, it made this very non-portable.
     * This method limits the maximum number of args. But is much more
     * portable.
     */
    switch (ct)
    {
    case CT_CDECL:
        switch (nargs)
        {
        case 0: return (*(long (*)())addr)();
        case 1: return (*(long (*)())addr)(av[0]);
        case 2: return (*(long (*)())addr)(av[0], av[1]);
        case 3: return (*(long (*)())addr)(av[0], av[1], av[2]);
        case 4: return (*(long (*)())addr)(av[0], av[1], av[2], av[3]);
        case 5: return (*(long (*)())addr)(av[0], av[1], av[2], av[3], av[4]);
        case 6: return (*(long (*)())addr)(av[0], av[1], av[2], av[3], av[4], av[5]);
        case 7: return (*(long (*)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6]);
        case 8: return (*(long (*)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6], av[7]);
        }
        break;

#       ifdef _WIN32
        case CT_STDCALL:
            switch (nargs)
            {
            case 0: return (*(long (__stdcall *)())addr)();
            case 1: return (*(long (__stdcall *)())addr)(av[0]);
            case 2: return (*(long (__stdcall *)())addr)(av[0], av[1]);
            case 3: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2]);
            case 4: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2], av[3]);
            case 5: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2], av[3], av[4]);
            case 6: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5]);
            case 7: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6]);
            case 8: return (*(long (__stdcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6], av[7]);
            }
            break;

        case CT_FASTCALL:
            switch (nargs)
            {
            case 0: return (*(long (__fastcall *)())addr)();
            case 1: return (*(long (__fastcall *)())addr)(av[0]);
            case 2: return (*(long (__fastcall *)())addr)(av[0], av[1]);
            case 3: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2]);
            case 4: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2], av[3]);
            case 5: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2], av[3], av[4]);
            case 6: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5]);
            case 7: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6]);
            case 8: return (*(long (__fastcall *)())addr)(av[0], av[1], av[2], av[3], av[4], av[5], av[6], av[7]);
            }
            break;
#       else
        default:
            abort();
#       endif
    }
    return 0;
}


/*
 * Is an object an array only containing string objects?
 */
static int
is_array_of_strings(object_t *o)
{
    array_t     *a;
    object_t    **po;

    if (!ici_isarray(o))
        return 0;
    a = ici_arrayof(o);
    for (po = ici_astart(a); po != ici_alimit(a); po = ici_anext(a, po));
        if (!ici_isstring(*po))
            return 0;
    return 1;
}

/*
 * Turn an ICI array of strings into an argv-style array. The memory for
 * the argv-style array is allocated (via zalloc()) and must be freed
 * after use. The memory is allocated as one block so a single zfree()
 * of the function result is all that's required.
 */
static char **
ici_array_to_argv(ici_array_t *a)
{
    char        **argv;
    int         argc;
    size_t      sz;
    char        *p;
    ici_obj_t   **po;
    int         i;

    argc = ici_array_nels(a);
    sz = (argc + 1) * sizeof (char *);
    for (po = ici_astart(a); po != ici_alimit(a); po = ici_anext(a, po));
    {
        if (!ici_isstring(*po))
        {
            ici_error = "non-string in argv-style array";
            return NULL;
        }
        sz += ici_stringof(*po)->s_nchars + 1;
    }
    if ((argv = ici_alloc(sz)) == NULL)
        return NULL;
    p = (char *)&argv[argc + 1];
    i = 0;
    for (po = ici_astart(a); po != ici_alimit(a); po = ici_anext(a, po));
    {
        argv[i++] = p;
        memcpy(p, ici_stringof(*po)->s_chars, ici_stringof(*po)->s_nchars + 1);
        p += ici_stringof(*po)->s_nchars + 1;
    }
    argv[argc] = NULL;
    return argv;
}

/*
 * Implement a function call into some dynamically chosen (by name) function
 * in a dynamically loaded DLL.
 *
 * When I first wrote this I assumed it would only ever be used on Windows
 * (pretty silly, I know) and I used alloca().  While available on most
 * systems, it's not ANSI, which is a bit of a problem.
 */
static int
call_dllfunc(ici_obj_t *object, ici_obj_t *subject)
{
    dllfunc_t           *df;
    int                 i;
    long                *args;
    long                *argp;
    int                 nargs; /* In 32 bit words. */
    long                ret;
    char                *s;
    object_t            *o;
#   ifdef NO_ALLOCA
        /*
         * This is a bit of a hack so we can at least work most of the time
         * on systems without alloca. If the limited size becomes a problem,
         * a more complete solution should be coded.
         */
        char            abuf[1024];
        char            *ap = abuf;
        char            *apt;
#       define alloca(n) (void *)(apt = ap, ap += (n), ap <= &abuf[1024] ? apt : NULL)
#   endif

    nargs = ICI_NARGS();
    df = dllfuncof(object);
    assert(isdllfunc(df));
    if (df->df_addr == NULL)
    {
        dll_t           *dll;
        calltype_t      ct;

        /*
         * We don't yet have the address of this function. Try and find
         * it by name in the dll. MS C compilers decorate names in different
         * ways depending on the calling convention and stuff. There is also
         * the issue of ASCII vs UNICODE versions. Try all the possibolities
         * until we find one that exists.
         *
         * Most of this is for the benefit of Windows.  But there is at least
         * one major problem.  A vararg function declared stdcall is
         * automatically changed to cdecl, but without changing the name, so
         * we can't tell.  For now, we just have to say don't call vararg
         * functions with this on Windows.
         */
        s = ici_alloc(df->df_name->s_nchars + 15);
        if (s == NULL)
            return 1;
        dll = df->df_dllib->dll_lib;
        ct = CT_CDECL;
        sprintf(s, "_%s", df->df_name->s_chars);
        if ((df->df_addr = dlsym(dll, s)) != NULL)
            goto got_addr;
        sprintf(s, "%s", df->df_name->s_chars);
        if ((df->df_addr = dlsym(dll, s)) != NULL)
            goto got_addr;
        ct = CT_STDCALL;
        sprintf(s, "_%s@%d", df->df_name->s_chars, nargs * 4);
        if ((df->df_addr = dlsym(dll, s)) != NULL)
            goto got_addr;
        sprintf(s, "%sA", df->df_name->s_chars);
        if ((df->df_addr = dlsym(dll, s)) != NULL)
            goto got_addr;
        ct = CT_FASTCALL;
        sprintf(s, "@%s@%d", df->df_name->s_chars, nargs * 4);
        if ((df->df_addr = dlsym(dll, s)) != NULL)
            goto got_addr;
        if (ici_chkbuf(3 * strlen(s) + 40))
        {
            ici_free(s);
            return 1;
        }
        sprintf(ici_buf, "could not locate symbol \"%s\"", df->df_name->s_chars);
        ici_error = ici_buf;
        ici_free(s);
        return 1;

    got_addr:
#   ifndef _WIN32
        ct = CT_CDECL;
#   endif
        if (df->df_calltype == CT_UNKNOWN)
            df->df_calltype = ct;
        ici_free(s);
    }
    /*
     * 1. Argument translation
     *
     * The linkage function will follow simple rules to translate actual
     * arguments made in the ICI call into actual arguments passed to the
     * library function.
     *
     * Up to 8 32 bit words worth of arguments may be passed to the
     * library function.
     *
     * int                  Will be passed as a 32 bit integer.
     *
     * string               Will be passed as a pointer to the internal
     *                      character data of the string. If the string
     *                      is atomic (as almost all strings are), the
     *                      function must not modify the content in any way.
     *                      If the string is non-atomic (as created by
     *                      'strbuf()') it may be modified within its
     *                      length constraints. Note that the internal
     *                      representation of ICI strings have a guard
     *                      nul character just beyond their known length,
     *                      and thus can be used as C nul-terminated strings.
     *                      But they may also have internal nul characters
     *                      of course.
     *
     * NULL                 A 32 bit 0 will be passed.
     *
     * array of strings     Will be conveted to an argv style NULL
     *                      terminated array of nul terminated strings
     *                      and passed to the function.
     *
     * &int                 A pointer to an int will be passed as
     *                      a pointer to a 32 bit integer value, where
     *                      that value may be updated by the called
     *                      function. Upon return, the updated value will
     *                      be converted to an ICI integer and the pointed
     *                      to ICI value updated.
     *
     * This --topic-- forms part of the --ici-dll-- documentation.
     */
 
    /*
     * Allocate and build an array of arguments as 32 bit words. Left-most
     * argument is first in array.
     */
    args = (long *)alloca(nargs * sizeof(long));
    if (args == NULL)
        goto alloca_fail;
    argp = &args[0];
    for (i = 0; i < ICI_NARGS(); ++i, ++argp)
    {
        o = ICI_ARG(i);
        if (ici_isint(o))
        {
            *argp = ici_intof(o)->i_value;
        }
        else if (ici_isstring(o))
        {
            *argp = (long)ici_stringof(o)->s_chars;
        }
        else if (ici_isnull(o))
        {
            *argp = 0;
        }
        else if (is_array_of_strings(o))
        {
            *argp = (long)ici_array_to_argv(ici_arrayof(o));
        }
        else if (ici_isptr(o))
        {
            ici_obj_t    *po;

            po = ici_fetch(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key);
            if (ici_isint(po))
            {
                long    *p;

                /*
                 * A pointer to an int. Allocate space for our real pointer
                 * to point to.
                 */
                p = (long *)alloca(sizeof(long));
                if (p == NULL)
                    goto alloca_fail;
                *p = ici_intof(po)->i_value;
                *argp = (long)p;
            }
            else if (is_array_of_strings(po))
            {
                char ***pargv;
                pargv = alloca(sizeof (char **));
                if (pargv == NULL)
                    goto alloca_fail;
                *pargv = ici_array_to_argv(ici_arrayof(po));
                *argp = (long)pargv;
            }
            else
            {
                return ici_argerror(i);
            }
        }
        else if (ici_ismem(o))
        {
            *argp = (long)ici_memof(o)->m_base;
        }
    }
    /*
     * 1. Return value translation
     *
     * Currently the return value is assumed to be a 32 bit int and
     * is returned as an ICI int.
     *
     * This --topic-- forms part of the --ici-dll-- documentation.
     */
    ret = call_long(df->df_calltype, df->df_addr, nargs, args);
    /*
     * Re-scan our arguments for any pointers (to int) and update the
     * value that they point to from the updated value from the
     * function (if it has changed).
     */
    argp = &args[0];
    for (i = 0; i < ICI_NARGS(); ++i, ++argp)
    {
        o = ICI_ARG(i);
        if (ici_isptr(o))
        {
            ici_obj_t    *po;

            po = ici_fetch(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key);
            if (ici_isint(po) && ici_intof(po)->i_value != *(long *)*argp)
            {
                if ((po = ici_objof(ici_int_new(*(long *)*argp))) == NULL)
                    return 1;
                if (ici_assign(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key, po))
                    return 1;
            }
        }
        else if (is_array_of_strings(o))
        {
            ici_free((char *)*argp);
        }
    }
    return ici_int_ret(ret);

alloca_fail:
    ici_error = "dll arguments occupy too much memory";
    return 1;
}

static unsigned long
mark_dllfunc(ici_obj_t *o)
{
    return sizeof(dllfunc_t)
        + ici_mark(dllfuncof(o)->df_dllib)
        + ici_mark(dllfuncof(o)->df_name)
        + (dllfuncof(o)->df_decl ? ici_mark(dllfuncof(o)->df_decl) : 0);
}

static void
free_dllfunc(ici_obj_t *o)
{
    ici_tfree(o, dllfunc_t);
}

/*
 * 1. Calling conventions - CALLTYPE
 *
 * Even within a single archietcture and operating system, there
 * are sometimes different calling conventions (meaning such things
 * as the manner of packing parameters onto the stack and how the
 * return value is passed back).
 *
 * By default, for any particular function, the 'dll' module will
 * in assume the normal calling convention that an unadorned C
 * declaration will produce unless the name is "decorated" in a manner
 * that indicates otherwise.
 *
 * In cases where even this fails (Windows API calls) it is possible
 * to "declare" that all the functions in a given library have a
 * certain calling convention by assigning that convention to the
 * 'CALLTYPE' field of the library. For example:
 *
 *  dll.user32.CALLTYPE = "winapi";
 *  dll.kernel32.CALLTYPE = "winapi";
 *  dll.advapi32.CALLTYPE = "winapi";
 *
 * This is currently only necessary for Windows. The values that can
 * be assigned are: 'stdcall' or 'winapi', which are equivalent,
 * 'fastcall', or 'cdecl'. 'cdecl' is the default.
 *
 * This --topic-- forms part of the --ici-dll-- documentation.
 */

/*
 * Make a new ICI func object.  The address of the actual function is not
 * discovered until first call.  That is in case we need to know anything
 * about the argument list to find the function name (as is the case in
 * Windows stdcall calling convention).
 */
dllfunc_t *
new_dllfunc(dllib_t *dllib, ici_str_t *name, ici_str_t *decl)
{
    dllfunc_t           *df;
    ici_obj_t           *a;

    if ((df = ici_talloc(dllfunc_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(df, dllfunc_tcode, 0, 1, 0);
    df->df_name = name;
    df->df_decl = decl;
    df->df_calltype = CT_UNKNOWN;
    df->df_dllib = dllib;
    df->df_addr = NULL;
    ici_rego(df);
    if ((a = ici_fetch(dllib->dll_struct, ICISO(CALLTYPE))) == NULL)
        return NULL;
    if (a == ICISO(stdcall) || a == ICISO(winapi))
        df->df_calltype = CT_STDCALL;
    else if (a == ICISO(fastcall))
        df->df_calltype = CT_FASTCALL;
    else if (a == ICISO(Xcdecl))
        df->df_calltype = CT_CDECL;
    return df;
}

ici_type_t  dllfunc_type =
{
    mark_dllfunc,
    free_dllfunc,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "dllfunc",
    NULL,
    call_dllfunc
};

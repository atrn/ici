#define ICI_NO_OLD_NAMES

#include <ici.h>
#include <icistr.h>
#include <icistr-setup.h>

static ici_obj_t        *dbg_module;
static int              dbg_funcs_got;
static ici_obj_t        *dbg_error_func;
static ici_obj_t        *dbg_src_func;
static ici_obj_t        *dbg_fncall_func;
static ici_obj_t        *dbg_fnresult_func;

/*
 * Get the pointers to the ICI coded functions. We have to do this
 * on first reference because they aren't parsed until after
 * ici_dbg_library_init returns.
 */
static int
dbg_get_funcs(void)
{
    if
    (
        (dbg_error_func = ici_fetch(dbg_module, ICISO(dbg_error))) == NULL
        ||
        (dbg_src_func = ici_fetch(dbg_module, ICISO(dbg_src))) == NULL
        ||
        (dbg_fncall_func = ici_fetch(dbg_module, ICISO(dbg_fncall))) == NULL
        ||
        (dbg_fnresult_func = ici_fetch(dbg_module, ICISO(dbg_fnresult))) == NULL
    )
        return 1;
    dbg_funcs_got = 1;
    return 0;
}

static void
dbg_fail(void)
{
    fprintf(stderr, "Error in ICI dbg module: %s\n", ici_error);
}

/*
 * ici_debug_error - called when the program raises an error.
 *
 * Parameters:
 *
 *      err     the error being set.
 *      src     the last source marker encountered.
 */
static void
ici_debug_error(char *err, ici_src_t *src)
{
    ici_debug_enabled = 0;
    if (!dbg_funcs_got && dbg_get_funcs())
    {
        dbg_fail();
        return;
    }
    if (ici_func(dbg_error_func, "soi", ici_error, src->s_filename, src->s_lineno))
    {
        dbg_fail();
        return;
    }
    ici_debug_enabled = 1;
}

/*
 * ici_debug_fncall - called prior to a function call.
 *
 * Parameters:
 *
 *      o       The function being called.
 *      ap      The parameters to function, a (C) array of objects.
 *      nargs   The number of parameters in that array.
 */
static void
ici_debug_fncall(ici_obj_t *o, ici_obj_t **ap, int nargs)
{
    ici_array_t         *a;

    ici_debug_enabled = 0;
    if (!dbg_funcs_got && dbg_get_funcs())
    {
        dbg_fail();
        return;
    }
    if ((a = ici_array_new(nargs)) == NULL)
    {
        dbg_fail();
        return;
    }
    while (nargs-- > 0)
        *a->a_top++ = *ap--;
    if (ici_func(dbg_fncall_func, "oo", o, a))
    {
        ici_decref(a);
        dbg_fail();
        return;
    }
    ici_decref(a);
    ici_debug_enabled = 1;
}

/*
 * ici_debug_fnresult - called upon function return.
 *
 * Parameters:
 *
 *      o       The result of the function.
 */
static void
ici_debug_fnresult(ici_obj_t *o)
{

    ici_debug_enabled = 0;
    if (!dbg_funcs_got && dbg_get_funcs())
    {
        dbg_fail();
        return;
    }
    if (ici_func(dbg_fnresult_func, "o", o))
    {
        dbg_fail();
        return;
    }
    ici_debug_enabled = 1;
}

/*
 * ici_debug_src - called when a source line marker is encountered.
 *
 * Parameters:
 *
 *      src     The source marker encountered.
 */
static void
ici_debug_src(ici_src_t *src)
{
    ici_debug_enabled = 0;
    if (!dbg_funcs_got && dbg_get_funcs())
    {
        dbg_fail();
        return;
    }
    if (ici_func(dbg_src_func, "oi", src->s_filename, src->s_lineno))
    {
        dbg_fail();
        return;
    }
    ici_debug_enabled = 1;
}

/*
 * ici_debug_watch - called upon each assignment.
 *
 * Parameters:
 *
 *      o       The object being assigned into. For normal variable
 *              assignments this will be a struct, part of the scope.
 *      k       The key being used, typically a string, the name of a
 *              variable.
 *      v       The value being assigned to the object.
 */
static void
ici_debug_watch(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
}

/*
 * The default debugging interface is the stub functions.  Debuggers
 * will assign ici_debug to point to a more useful set of functions.
 */
static ici_debug_t ici_debug_funcs =
{
    ici_debug_error,
    ici_debug_fncall,
    ici_debug_fnresult,
    ici_debug_src,
    ici_debug_watch
};


ici_obj_t *
ici_dbg_library_init(void)
{
    static ici_cfunc_t cfuncs[] =
    {
        /*
        {CF_OBJ, "break",       f_debug_break      },
        {CF_OBJ, "view",        f_WIDB_view_object },
        */
        {ICI_CF_OBJ}
    };

    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "net"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((dbg_module = ici_objof(ici_module_new(cfuncs))) == NULL)
        return NULL;
    ici_debug = &ici_debug_funcs;
    return dbg_module;
}

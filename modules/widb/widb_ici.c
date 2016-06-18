#include <ici.h>
#include <windows.h>
#include "widb_ici.h"
#include "widb-priv.h"
#include "widb.h"
#include "widb_wnd.h"


/*
 * Remembers whether we've performed necessary initialisation.
 */
BOOL widb_ici_initialised = FALSE;


/*
 * This array contains the current execution stack, it may be shown to
 * the user by the main window.
 *
 * The code inside widb_ici.c keeps this up-to-date.
 */
ici_array_t *widb_exec_name_stack = NULL;
static int exec_no_name_count[1000];       // Our recursion is limited.
static int exec_no_name_index = 0;


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
    if (widb_step_over_depth <= 0)
    {
        // Remember the scope in case they want to look at the variables.
        if (ici_vs.a_top == ici_vs.a_bot)
            widb_scope = objof(&o_null);
        else
            widb_scope = ici_vs.a_top[-1];
        ici_incref(widb_scope);
        // Stop executing and show the debugger until the user opts to continue in
        // some way.
        ici_incref(src);
        widb_debug(src);
        ici_decref(src);

        ici_decref(widb_scope);
        widb_scope = NULL;
    }
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
    char        n1[30];
    ici_str_t   *name = NULL;

    // Does the function have a valid name?  Only for ICI functions.
    if (o != NULL && isptr(o))
    {
        ici_obj_t *agg = ptrof(o)->p_aggr;
        o = ici_fetch(agg, ptrof(o)->p_key);
    }
    if (o != NULL)
    {
        //
        // The function has a name, that means it should appear in the user's
        // stack trace.
        //

        // Ensure that the stack exists.
        if (widb_exec_name_stack == NULL)
            widb_exec_name_stack = ici_array_new(0);

        // Make a new entry.
        VERIFY(0 == ici_stk_push_chk(widb_exec_name_stack, 1));
        ici_objname(n1, o);
        VERIFY(NULL != (name = ici_str_get_nul_term(n1)));
        *widb_exec_name_stack->a_top++ = objof(name);

        ++ exec_no_name_index;
    }
    else
    {
        //
        // The function has no name, but we must count the number of these
        // unamed functions so that we can detect when named functions
        // eventually exit (by counting the number of f__debug_return() calls).
        //
        ++ exec_no_name_count[exec_no_name_index];
    }

    // The depth is used by the window.
    widb_step_over_depth ++;
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
    if (widb_exec_name_stack != NULL)
    {
        if (exec_no_name_count[exec_no_name_index] == 0)
        {
            // This must be a named function returning, pop it from the execution
            // stack.
            //
            // Sometimes we've noticed with Rama that it pops beyond the
            // beginning of the stack.  Perhaps this has to do with debugging
            // being invoked at a lower level.  Not sure, not important enough
            // to investigate.  Instead we'll just ignore such things.
            if (ici_array_nels(widb_exec_name_stack) > 0)
            {
                // Popping available.
                ici_array_pop(widb_exec_name_stack);

                -- exec_no_name_index;
                VERIFY(exec_no_name_index >= 0);
            }
        }
        else
        {
            // Just another unnamed function returning.
            -- exec_no_name_count[exec_no_name_index];
        }

        // The depth is used by the window.
        widb_step_over_depth --;
    }
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
    char debug_str[256];
    char *filename = "";

    // Translate it into the standard Visual C++ form so that the user can
    // double-click on it.
    if
    (
        src->s_filename != NULL
        &&
        isstring(objof(src->s_filename))
        &&
        src->s_lineno != 0
    )
    {
        filename = src->s_filename->s_chars;
    }
    sprintf(debug_str, "%s(%ld): %s\n", filename, src->s_lineno, err);
    OutputDebugString(debug_str);

    // We'll present the exception just like MFC does.
    switch (MessageBox(widb_wnd, err, "ICI Exception", MB_ICONERROR | MB_ABORTRETRYIGNORE))
    {
        case IDABORT:
            ExitProcess(1);

        case IDRETRY:
            // Remember the scope in case they want to look at the variables.
            if (ici_vs.a_top == ici_vs.a_bot)
                widb_scope = objof(&o_null);
            else
                widb_scope = ici_vs.a_top[-1];
            ici_incref(widb_scope);

            // Stop executing and show the debugger until the user opts to continue in
            // some way.
            ici_incref(src);
            widb_debug(src);
            ici_decref(src);
            ici_decref(widb_scope);
            widb_scope = NULL;
            break;

        case IDIGNORE:
            break;
    }
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
 * The interface to the debugging functions.  This replaces the default
 * interface which consists only of stub functions.
 */
ici_debug_t ici_debug_funcs =
{
    ici_debug_error,
    ici_debug_fncall,
    ici_debug_fnresult,
    ici_debug_src,
    ici_debug_watch
};

/*
 * widb.break()
 *
 * Suspends execution and presents the debugger's GUI to the user,
 * where they may view the current source, the stact traceback,
 * and variables.
 *
 * This --topic-- forms part of the --ici-widb-- documentation.
 */
static int
f_debug_break()
{
    // This will ensure that we stop at the very next line.
    widb_step_over_depth = -99999;

    return ici_null_ret();
}


/*
 * widb.view(any)
 *
 * Suspends executions and presents a window to the user with the value
 * 'any' displayed. If 'any' is an aggregate object, it can be expanded
 * and navigated.
 *
 * This --topic-- forms part of the --ici-widb-- documentation.
 */
static int
f_WIDB_view_object()
{
    ici_obj_t *o;
    if (ici_typecheck("o", &o))
        return 1;
    WIDB_view_object(o, NULL);
    return ici_null_ret();
}


/*
 * widb_ici_uninit
 *
 *  Cleans up anything allocated by this module.
 */
static void
widb_ici_uninit(void)
{
    if (widb_exec_name_stack != NULL)
    {
        ici_decref(widb_exec_name_stack);
        widb_exec_name_stack = NULL;
    }
}

/*
 * Windows ICI debugger
 *
 * The Windows ICI Debugger ('widb') extension module provides a simple
 * graphical interface to ICI's debugging facilities under Windows.
 *
 * From the command line, the module can be pre-loaded by using the
 * '-l' option, as in:
 *
 *  ici -l widb -f myprog.ici args...
 *
 * Or loaded within a program with:
 *
 *  load("widb");
 *
 * Upon load, the extension module self-registers with ICI's debugger
 * interface and enables debugging.
 *
 * Uncaught errors will automatically trigger the debugger and an alert
 * box with the error and the choices "Abort", "Retry", and "Ignore".
 * Selecting "Retry" will cause the debuggers GUI to be presented
 * with a source window at the failing source line.
 *
 * The debugger may also be triggered explicitly by use of the 'widb.break'
 * and 'widb.view' functions.
 *
 * This debugger is fairly old. It is due for an update.
 *
 * This --intro-- and --synopsis-- forms part of --ici-widb-- documentation.
 */
ici_obj_t *
ici_widb_library_init() /* Was widb_ici_init() */
{
    static ici_wrap_t   wrap;
    ici_objwsup_t       *s;

    static ici_cfunc_t cfuncs[] =
    {
        {CF_OBJ, "break",       f_debug_break      },
        {CF_OBJ, "view",        f_WIDB_view_object },
        {CF_OBJ}
    };

    if ((s = ici_module_new(cfuncs)) == NULL)
        return NULL;
    ici_debug = &ici_debug_funcs;
    ici_debug_enabled = 1;
    /* To save users the effort of enabling profiling, we'll do it for them. */
    WIDB_enable_profiling_display();

    /* Ensure that this module is uninitialised on exit. */
    ici_atexit(widb_ici_uninit, &wrap);
    return objof(s);
}


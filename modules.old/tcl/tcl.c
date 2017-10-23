/*
 * tcl.c - interface between ICI and Tcl/Tk.
 *
 * Andy Newman <an@atrn.org>
 *
 */

/*
 * The ICI tcl module is a binding beween the ICI and TCL languages
 * permitting each language to invoke code in the other.  The primary
 * use of the TCL module is to allow ICI code access to TCL's TK GUI
 * toolkit.
 *
 * This --intro-- and --synopsis-- are part of --ici-tcl-- documentation.
 */


#define ICI_NO_OLD_NAMES

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include <tcl.h>
#ifndef ICI_TCL_WITHOUT_TK
#include <tk.h>
#endif

/*
 * The default TCL interpreter instance, created when the module is loaded.
 */
static Tcl_Interp *default_interp;

/*
 * An "ici" command for Tcl letting you execute ICI code from Tcl.
 */
static int
ici_cmd(ClientData data, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc != 2)
    {
        interp->result = "wrong # args, should be: ici string";
        return TCL_ERROR;
    }
    if (ici_call(ICIS(parse), "s", argv[1]))
    {
        interp->result = ici_error;
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * Set the ici error string from the TCL interpreter's result string
 * or, if that's too big, from some default message.
 */
static int
set_error(Tcl_Interp *interp, const char *default_error)
{
    if (ici_chkbuf(strlen(interp->result) + 1))
        ici_error = (char *)default_error;
    else
    {
        strcpy(ici_buf, interp->result);
        ici_error = ici_buf;
    }
    return 1;
}

/*
 * Create a new Tcl interpreter structure and return its handle.
 *
 * This is called from a couple of places, when creating a new
 * tcp_interp object for ICI and when creating the default interpreter
 * object. This is the only place where the various Tcl extension's
 * "Init" routines get called so all interpreter's are alike (i.e.,
 * have Tk and ICI extensions).
 */
static Tcl_Interp *
make_tcl_interp(void)
{
    Tcl_Interp  *interp;

    if ((interp = Tcl_CreateInterp()) == NULL)
    {
        ici_error = "failed to create TCL interpreter instance";
        return NULL;
    }
    if (Tcl_Init(interp) == TCL_ERROR)
	goto fail;
#ifndef ICI_TCL_WITHOUT_TK
    if (Tk_Init(interp) == TCL_ERROR)
        goto fail;
#endif
    if (Tcl_CreateCommand(interp, "ici", ici_cmd, NULL, NULL) == NULL)
        goto fail;

    return interp;

fail:
    set_error(interp, "TCL interpreter failed to initialize");
    Tcl_DeleteInterp(interp);
    return NULL;
}

/*
 * Callback function to cleanup the TCL interpreter.  This is attached to
 * the ICI handle object that represents the TCL interpreter and invoked
 * when the ICI garbage collector destroys that object.
 */
static void
delete_tcl_interp(ici_handle_t *h)
{
    Tcl_DeleteInterp(h->h_ptr);
}

/*
 * Name to identifier mapping table used by the ICI handle object to
 * allow member access via its "member_intf" function.
 *
 * We define two such members,
 *
 *      result          The TCL command's result, a string.
 *      error           The TCL error line number, an integer.
 */

enum
{
    RESULT = 1,
    ERROR  = 2
};

static ici_name_id_t tcl_interp_members[] =
{
    {"result", RESULT},
    {"error",  ERROR},
    {NULL, 0}
};

/*
 * Function invoked by the ICI handle object to perform member accesses
 * on the underlying object referenced from the handle.
 */
static int
tcl_interp_member_intf(void *ptr, int id, ici_obj_t *setv, ici_obj_t **retv)
{
    Tcl_Interp *interp = ptr;

    /* If setv is non-null an assigment is being attempted. We don't do that. */
    if (setv != NULL)
    {
        ici_error = "attempt to assign to a TCL interpreter member";
        return 1;
    }
    switch (id)
    {
    case RESULT:
        *retv = ici_objof(ici_str_new_nul_term(interp->result));
        break;
    case ERROR:
        *retv = ici_objof(ici_int_new(interp->errorLine));
        break;
    default:
        ici_error = "unrecognised key for TCL interpreter object";
        return 1;
    }
    return 0;
}

/*
 * Create a new tcl_interp object.
 */
static ici_obj_t *
new_tcl_interp(void)
{
    Tcl_Interp          *tcl_interp;
    ici_handle_t        *h;

    if ((tcl_interp = make_tcl_interp()) == NULL)
        return NULL;
    if ((h = ici_handle_new(tcl_interp, ICIS(tcl_interp), NULL)) == NULL)
        goto fail;
    if ((h->h_member_map = ici_make_handle_member_map(tcl_interp_members)) == NULL)
    {
        ici_decref(h);
        h = NULL;
    }
    h->h_member_intf = tcl_interp_member_intf;
    h->h_pre_free = delete_tcl_interp;
    return ici_objof(h);

fail:
    Tcl_DeleteInterp(tcl_interp);
    return NULL;
}

/*
 * tcl_interp = tcl.interpreter()
 *
 * Create and return a new TCL interpreter object.
 *
 * The returned interpreter has a TCL "ici" command for invoking
 * ICI from within TCL code.
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */
static int
f_tcl_interpreter(void)
{
    return ici_ret_with_decref(ici_objof(new_tcl_interp()));
}

/*
 * string = Eval([interpreter,] string)
 * string = GlobalEval([interpreter,] string)
 * string = EvalFile([interpreter,] string)
 *
 * Evaluate Tcl code and return the result. The nature of evaluation depends
 * on the actual function being called and controls how the string parameter
 * is used. Eval and GlobalEval evaluate code directly and the string contains
 * Tcl code. EvalFile evalulates the content of a file and the string is the
 * name of the file containing the Tcl program to be evaluated.
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */

/*
 * Machinery to permit us to use a single, parameterized, function to
 * invoke the various forms of TCL's eval functions.
 *
 * This is all needed as ANSI/ISO C does not permit conversion between
 * function pointers and void * and the ici_cfunc_t has a pointer to void.
 * So we use double indirection and store a pointer to a struct which
 * has the pointer to function.  For extra safety we also define an id
 * used when initializing the cfuncs.
 */

enum
{
    EVAL = 0,
    GLOBAL_EVAL = 1,
    EVAL_FILE = 2
};

struct eval_func
{
    int         (*fn)(Tcl_Interp *, const char *);
}
eval_funcs[] =
{
    {Tcl_Eval},
    {Tcl_GlobalEval},
    {Tcl_EvalFile}
};

static int
f_tcl_eval(void)
{
    ici_handle_t        *h;
    Tcl_Interp          *interp;
    char                *str;
    struct eval_func    *eval = ICI_CF_ARG1();

    switch (ICI_NARGS())
    {
    case 2:
        if (ici_typecheck("hs", ICIS(tcl_interp), &h, &str))
            return 1;
        interp = h->h_ptr;
        break;

    case 1:
        if (ici_typecheck("s", &str))
            return 1;
        interp = default_interp;
        break;

    default:
        return ici_argcount(2);
    }
    if ((*eval->fn)(interp, str) != TCL_OK)
    {
        return set_error(interp, "undiagnosed error in TCL interpereter");
    }
    return ici_str_ret(interp->result);
}

/*
 * string = GetVar([interp,] name [, "global" ])
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */
static int
f_tcl_get_var(void)
{
    Tcl_Interp          *interp;
    ici_handle_t        *h;
    int                 flags = 0;
    char                *name;
    ici_str_t           *str;
    const char          *s;

    switch (ICI_NARGS())
    {
    case 1:
        if (!ici_isstring(ICI_ARG(0)))
            return ici_argerror(0);
        name = ici_stringof(ICI_ARG(0))->s_chars;
        interp = default_interp;
            return 1;
        flags = 0;
        break;

    case 2:
        if (ici_ishandle(ICI_ARG(0)))
        {
            if (ici_typecheck("hs", ICIS(tcl_interp), &h, &name))
                return 1;
            interp = h->h_ptr;
        }
        else
        {
            if (ici_typecheck("so", &name, &str))
                return 1;
            if (!ici_isstring(ici_objof(str)) || str != ICIS(global))
                return ici_argerror(1);
            flags = TCL_GLOBAL_ONLY;
            interp = default_interp;
        }
        break;

    case 3:
        if (ici_typecheck("hso", ICIS(tcl_interp), &h, &name, &str))
            return 1;
        if (!ici_isstring(ici_objof(str)) || str != ICIS(global))
            return ici_argerror(2);
        interp = h->h_ptr;
        flags = TCL_GLOBAL_ONLY;
        break;

    default:
        return ici_argcount(3);
    }
    if ((s = Tcl_GetVar(interp, name, flags | TCL_LEAVE_ERR_MSG)) == NULL)
    {
        return set_error(interp, "failed to fetch TCL variable");
    }
    return ici_str_ret((char *)s);
}

/*
 * string = GetVar2([interp,] name, name [, "global" ])
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */
static int
f_tcl_get_var2(void)
{
    Tcl_Interp          *interp;
    ici_handle_t        *h;
    int                 flags = 0;
    char                *name;
    char                *name2;
    ici_str_t           *str;
    const char          *s;

    switch (ICI_NARGS())
    {
    case 2:
        if (ici_typecheck("ss", &name, &name2))
            return 1;
        interp = default_interp;
        flags = 0;
        break;

    case 3:
        if (ici_ishandle(ICI_ARG(0)))
        {
            if (ici_typecheck("hss", ICIS(tcl_interp), &h, &name, &name2))
                return 1;
            interp = h->h_ptr;
        }
        else
        {
            if (ici_typecheck("sso", &name, &name2, &str))
                return 1;
            if (!ici_isstring(ici_objof(str)) || str != ICIS(global))
                return ici_argerror(1);
            flags = TCL_GLOBAL_ONLY;
            interp = default_interp;
        }
        break;

    case 4:
        if (ici_typecheck("hsso", ICIS(tcl_interp), &h, &name, &name2, &str))
            return 1;
        if (!ici_isstring(ici_objof(str)) || str != ICIS(global))
            return ici_argerror(2);
        interp = h->h_ptr;
        flags = TCL_GLOBAL_ONLY;
        break;

    default:
        return ici_argcount(3);
    }
    if ((s = Tcl_GetVar2(interp, name, name2, flags | TCL_LEAVE_ERR_MSG)) == NULL)
    {
        return set_error(interp, "failed to get TCL variable");
    }
    return ici_str_ret((char *)s);
}

/*
 * SetVar([interpreter], name, value)
 * SetVar([interpreter], name, name2, value)
 *
 * Set the Tcl variable name or the Tcl array variable name(name2) to
 * the given value. All objects are strings.
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */
static int
f_tcl_set_var(void)
{
    char                *name;
    char                *name2 = NULL;
    char                *value;
    ici_handle_t        *h;
    Tcl_Interp          *interp;

    interp = default_interp;
    switch (ICI_NARGS())
    {
    case 2:
        if (ici_typecheck("ss", &name, &value))
            return 1;
        break;

    case 3:
        if (ici_ishandle(ICI_ARG(0)))
        {
            if (ici_typecheck("hss", ICIS(tcl_interp), &h, &name, &value))
                return 1;
            interp = h->h_ptr;
        }
        else
        {
            if (ici_typecheck("sss", &name, &name2, &value))
                return 1;
        }
        break;

    case 4:
        if (ici_typecheck("hsss", ICIS(tcl_interp), &h, &name, &name2, &value))
            return 1;
        interp = h->h_ptr;
        break;

    default:
        return ici_argerror(3);
    }
    if (name2 == NULL)
    {
        if (Tcl_SetVar(interp, name, value, TCL_LEAVE_ERR_MSG) == NULL)
            goto fail;
    }
    else
    {
        if (Tcl_SetVar2(interp, name, name2, value, TCL_LEAVE_ERR_MSG) == NULL)
            goto fail;
    }
    return ici_null_ret();

fail:
    return set_error(interp, "failed to set TCL variable");
}

#ifndef ICI_TCL_WITHOUT_TK
/*
 * tcl.tk_main_loop()
 *
 * Run Tk's event processing loop.
 *
 * This --topic-- forms part of the --ici-tcl-- documentation.
 */
static int
f_tk_main_loop(void)
{
    Tk_MainLoop();
    return ici_null_ret();
}
#endif

/*
 * Table of functions we export. Used either during link by referencing
 * tcl_cfuncs from conf.c or during dynamic loading via icitcl_library_init()
 * (see below).
 */
static ici_cfunc_t
cfuncs[] =
{
    {ICI_CF_OBJ,    "Interpreter",  f_tcl_interpreter},
    {ICI_CF_OBJ,    "Eval",         f_tcl_eval,             &eval_funcs[EVAL]},
    {ICI_CF_OBJ,    "GlobalEval",   f_tcl_eval,             &eval_funcs[GLOBAL_EVAL]},
    {ICI_CF_OBJ,    "EvalFile",     f_tcl_eval,             &eval_funcs[EVAL_FILE]},
    {ICI_CF_OBJ,    "GetVar",       f_tcl_get_var},
    {ICI_CF_OBJ,    "GetVar2",      f_tcl_get_var2},
    {ICI_CF_OBJ,    "SetVar",       f_tcl_set_var},
#ifndef ICI_TCL_WITHOUT_TK
    {ICI_CF_OBJ,    "tk_main_loop", f_tk_main_loop},
#endif
    {ICI_CF_OBJ}
};

/*
 * This is called by the ICI interpreter when this module is loaded.
 * It is the only public symbol exported by the module.
 */
ici_obj_t *
ici_tcl_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "tcl"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((default_interp = make_tcl_interp()) == NULL)
        return NULL;
    return ici_objof(ici_module_new(cfuncs));
}

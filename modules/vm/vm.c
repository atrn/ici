/* #define ICI_NO_OLD_NAMES */

#define DEBUG 0

#if DEBUG
#include <stdio.h>
static FILE *debug_out;
#endif

/*
 * The vm module ("virtual machine") permits users to inspect and create
 * the ICI virtual machine code created by the ICI interpreter.  Normally
 * the ICI VM is hidden from end-users.
 *
 * The module functions by patching new functions into the various object
 * types used by the interpreter.  Because of the way ICI is implemented,
 * where VM instructions share the same basic object structure as all other
 * ICI objects, this is sufficient to expose the internal types to ICI
 * programs.
 *
 * This --synopsis-- is part of the --ici-vm-- module.
 */

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

/*
 * This gumpf is required so we can include ICI's parse.h to pick up
 * the definitions of the various parse token constants.  These are
 * not normally exposed in the API and we're including an interpreter
 * header file that is not meant to be included by us so we have to
 * some special things.
 *
 * The file will try to include ICI's fwd.h who's content is already
 * included in the ici.h file we included above.  So we define the header
 * protection macro fwd.h uses to stop it redefining things.  The same is
 * done for ICI's object.h.   Because we exclude fwd.h we don't have a
 * required typedef so we do that too.  Finally we can include parse.h.
 */
#define ICI_FWD_H
#define ICI_OBJECT_H
typedef struct expr expr_t;
#include <parse.h>

/*
 * The "operator" type provides user level access to ICI's "op" type.
 * and is used for constructing op's at run-time.
 */

typedef struct
{
    ici_obj_t	o_head;
    ici_op_t	*o_op;
}
operator_t;

static int operator_tcode;

static __inline operator_t *
operatorof(void *p)
{
    return (operator_t *)p;
}

static unsigned long
mark_operator(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (operator_t) + ici_mark(operatorof(o)->o_op);
}

static void
free_operator(ici_obj_t *o)
{
    ici_tfree(o, operator_t);
}

static int
assign_operator(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    if (k == ici_objof(ICIS(code)))
    {
	if (!ici_isint(v))
	{
	    ici_error = "attempt to set operator code to a non-integer value";
	    return 1;
	}
	operatorof(o)->o_op->op_code = ici_intof(v)->i_value;
	return 0;
    }
    return ici_assign_fail(o, k, v);
}

static ici_obj_t *
fetch_operator(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_objof(ICIS(code)))
	return ici_objof(ici_int_new(operatorof(o)->o_op->op_code));
    if (k == ici_objof(ICIS(ecode)))
	return ici_objof(ici_int_new(operatorof(o)->o_op->op_ecode));
    return ici_fetch_fail(o, k);
}

static ici_type_t
operator_type =
{
    mark_operator,
    free_operator,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    assign_operator,
    fetch_operator,
    "operator"
};

static __inline int
isoperator(ici_obj_t *o)
{
    return o->o_tcode == operator_tcode;
}

static operator_t *
new_operator(ici_op_t *op)
{
    operator_t	*opr;

    if (op == NULL)
	return NULL;
    if ((opr = ici_talloc(operator_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(opr, operator_tcode, 0, 1, 0);
	ici_rego(opr);
    }
    return opr;
}

/*
 * A "sourcemarker" type for constructing src_t's
 */

typedef struct
{
    ici_obj_t	o_head;
    ici_src_t	*s_src;
}
sourcemarker_t;

static __inline sourcemarker_t *
sourcemarkerof(void *p)
{
    return (sourcemarker_t *)p;
}

static unsigned long
mark_sourcemarker(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (sourcemarker_t) + ici_mark(sourcemarkerof(o)->s_src);
}

static void
free_sourcemarker(ici_obj_t *o)
{
    ici_tfree(o, sizeof (sourcemarker_t));
}

static ici_obj_t *
fetch_sourcemarker(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_objof(ICIS(filename)))
	return ici_objof(sourcemarkerof(o)->s_src->s_filename);
    if (k == ici_objof(ICIS(lineno)))
    {
	ici_int_t	*rc;

	if ((rc = ici_int_new(sourcemarkerof(o)->s_src->s_lineno)) != NULL)
	    ici_decref(rc);
	return ici_objof(rc);
    }
    return ici_fetch_fail(o, k);
}

static ici_type_t	sourcemarker_type =
{
    mark_sourcemarker,
    free_sourcemarker,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    fetch_sourcemarker,
    "sourcemarker"
};

static int sourcemarker_tcode;

static __inline int
issourcemarker(ici_obj_t *o)
{
    return o->o_tcode == sourcemarker_tcode;
}

static sourcemarker_t *
new_sourcemarker(ici_src_t *src)
{
    sourcemarker_t	*s;
    
    if ((s = ici_talloc(sourcemarker_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(s, sourcemarker_tcode, 0, 1, 0);
	s->s_src = src;
	ici_rego(s);
    }
    return s;
}

/*
 * Given an operator, return a string giving its name
 */
static ici_str_t *
opname(ici_op_t *op)
{
    switch (op->op_ecode)
    {
    case ICI_OP_OTHER:
#include "ops-other.i"
	break;
#include "ops.i"
    }

    ici_error = "unknown operator";
    return NULL;
}

/*
 * Fetch function for op_t patched into type via vm.expose()
 */
static ici_obj_t *
fetch_op(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_objof(ICIS(code)))
	return ici_objof(ici_int_new(ici_opof(o)->op_code));
    if (k == ici_objof(ICIS(ecode)))
	return ici_objof(ici_int_new(ici_opof(o)->op_ecode));
    if (k == ici_objof(ICIS(name)))
	return ici_objof(opname(ici_opof(o)));
    return ici_fetch_fail(o, k);
}

/*
 * Fetch function for src_t patched into type via vm.expose()
 */
static ici_obj_t *
fetch_src(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_objof(ICIS(filename)))
	return ici_objof(ici_srcof(o)->s_filename);
    if (k == ici_objof(ICIS(lineno)))
    {
	ici_int_t	*rc;

	if ((rc = ici_int_new(ici_srcof(o)->s_lineno)) != NULL)
	    ici_decref(rc);
	return ici_objof(rc);
    }
    return ici_fetch_fail(o, k);
}


/*
 * Place to remember previous func_t fetch function, it isn't exported.
 * Set in vm.expose().
 */
static ici_obj_t	*(*fetch_func)(ici_obj_t *, ici_obj_t *) = NULL;

/*
 * Enhanced fetch function for func_t patched via vm.expose()
 */
static ici_obj_t *
fetch_func_ex(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_objof(ICIS(code)))
    {
#if DEBUG
	if (debug_out)
	    fprintf(debug_out, "returning code array %p for function %p\n", ici_funcof(o)->f_code, o);
#endif
	return ici_objof(ici_funcof(o)->f_code);
    }
    return (*fetch_func)(o, k);
}

/**
 * vm.expose()
 *
 * Patch the interpreter to allow user level access to internals
 * of various objects. Specifically it allows access to the compiled
 * ICI code and its special objects.
 */
static int
f_expose(void)
{
    if (fetch_func == NULL)
    {
#if DEBUG
	if (debug_out)
	    fprintf(debug_out, "patching fetch functions\n");
#endif
	fetch_func = ici_types[ICI_TC_FUNC]->t_fetch;
	ici_types[ICI_TC_OP]->t_fetch = fetch_op;
	ici_types[ICI_TC_SRC]->t_fetch = fetch_src;
	ici_types[ICI_TC_FUNC]->t_fetch = fetch_func_ex;
    }
    return ici_null_ret();
}

/**
 * sourcemarker = sourcemarker(src)
 * sourcemarker = sourcemarker(string, int)
 *
 * Create a sourcemarker object from a src object or a filename/line pair.
 */
static int
f_sourcemarker(void)
{
    ici_src_t		*src;
    ici_str_t		*filename;
    long		lineno;
    int			decit = 0;
    sourcemarker_t	*sm;
    
    if (ICI_NARGS() == 1)
	src = ici_srcof(ICI_ARG(0));
    else if (ici_typecheck("oi", &filename, &lineno))
	return 1;
    else if (!ici_isstring(ici_objof(filename)))
	return ici_argerror(0);
    else if ((src = ici_src_new(lineno, filename)) == NULL)
	return 1;
    else
	decit = 1;
    if (!ici_issrc(ici_objof(src)))
	return ici_argerror(0);
    sm = new_sourcemarker(src);
    if (decit)
	ici_decref(src);
    return ici_ret_with_decref(ici_objof(sm));
}

/**
 * operator = operator(op)
 * operator = operator(string)
 *
 * Construct an operator.
 */
static int
f_operator(void)
{
    ici_obj_t	*o;

    if (ICI_NARGS() != 1)
	return ici_argcount(1);
    o = ICI_ARG(0);
    if (ici_isop(o))
	return ici_ret_with_decref(ici_objof(new_operator(ici_opof(o))));
    if (!ici_isstring(o))
	return ici_argerror(0);

    if (o == ici_objof(ICIS(call)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_CALL, 0))));
    if (o == ici_objof(ICIS(namelvalue)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_NAMELVALUE, 0))));
    if (o == ici_objof(ICIS(dot)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_DOT, 0))));
    if (o == ici_objof(ICIS(dotkeep)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_DOTKEEP, 0))));
    if (o == ici_objof(ICIS(dotrkeep)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_DOTRKEEP, 0))));
    if (o == ici_objof(ICIS(assign)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_ASSIGN, 0))));
    if (o == ici_objof(ICIS(assignlocal)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_ASSIGNLOCAL, 0))));
    if (o == ici_objof(ICIS(exec)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_EXEC, 0))));
    if (o == ici_objof(ICIS(loop)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_LOOP, 0))));
    if (o == ici_objof(ICIS(ifnotbreak)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_IFNOTBREAK, 0))));
    if (o == ici_objof(ICIS(break)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_BREAK, 0))));
    if (o == ici_objof(ICIS(quote)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_QUOTE, 0))));
    if (o == ici_objof(ICIS(binop)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_BINOP, 0))));
    if (o == ici_objof(ICIS(at)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_AT, 0))));
    if (o == ici_objof(ICIS(swap)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_SWAP, 0))));
    if (o == ici_objof(ICIS(binop_for_temp)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_BINOP_FOR_TEMP, 0))));
    if (o == ici_objof(ICIS(aggr_key_call)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_AGGR_KEY_CALL, 0))));
    if (o == ici_objof(ICIS(colon)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_COLON, 0))));
    if (o == ici_objof(ICIS(coloncaret)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_COLONCARET, 0))));
    if (o == ici_objof(ICIS(method_call)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_METHOD_CALL, 0))));
    if (o == ici_objof(ICIS(super_call)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_SUPER_CALL, 0))));
    if (o == ici_objof(ICIS(assignlocalvar)))
	return ici_ret_with_decref(ici_objof(new_operator(ici_new_op(NULL, ICI_OP_ASSIGNLOCALVAR, 0))));

    return ici_argerror(0);
}

/*
 * string = vm.binop(op)
 * string = vm.binop(operator)
 *
 * Return the token for a binary operator.
 */
static int
f_binop(void)
{
    ici_op_t	*op;

    if (ICI_NARGS() != 1)
	return ici_argcount(0);
    if (isoperator(ICI_ARG(0)))
	op = operatorof(ICI_ARG(0))->o_op;
    else if (ici_isop(ICI_ARG(0)))
	op = ici_opof(ICI_ARG(0));
    else
	return ici_argerror(0);
    switch (op->op_code)
    {
    case t_subtype(T_ASTERIX):
	return ici_str_ret("*");
    case t_subtype(T_SLASH):
	return ici_str_ret("/");
    case t_subtype(T_PERCENT):
	return ici_str_ret("%");
    case t_subtype(T_PLUS):
	return ici_str_ret("+");
    case t_subtype(T_MINUS):
	return ici_str_ret("-");
    case t_subtype(T_GRTGRT):
	return ici_str_ret(">>");
    case t_subtype(T_LESSLESS):
	return ici_str_ret("<<");
    case t_subtype(T_LESS):
	return ici_str_ret("<");
    case t_subtype(T_GRT):
	return ici_str_ret(">");
    case t_subtype(T_LESSEQ):
	return ici_str_ret("<=");
    case t_subtype(T_GRTEQ):
	return ici_str_ret(">=");
    case t_subtype(T_EQEQ):
	return ici_str_ret("==");
    case t_subtype(T_EXCLAMEQ):
	return ici_str_ret("!=");
    case t_subtype(T_TILDE):
	return ici_str_ret("~");
    case t_subtype(T_EXCLAMTILDE):
	return ici_str_ret("!~");
    case t_subtype(T_2TILDE):
	return ici_str_ret("~~");
    case t_subtype(T_3TILDE):
	return ici_str_ret("~~~");
    case t_subtype(T_AND):
	return ici_str_ret("&");
    case t_subtype(T_CARET):
	return ici_str_ret("^");
    case t_subtype(T_BAR):
	return ici_str_ret("|");
    case t_subtype(T_ANDAND):
	return ici_str_ret("&&");
    case t_subtype(T_BARBAR):
	return ici_str_ret("||");
    case t_subtype(T_COLON):
	return ici_str_ret(":");
    case t_subtype(T_QUESTION):
	return ici_str_ret("?");
    case t_subtype(T_EQ):
	return ici_str_ret("=");
    case t_subtype(T_COLONEQ):
	return ici_str_ret(":=");
    case t_subtype(T_PLUSEQ):
	return ici_str_ret("+=");
    case t_subtype(T_MINUSEQ):
	return ici_str_ret("-=");
    case t_subtype(T_ASTERIXEQ):
	return ici_str_ret("*=");
    case t_subtype(T_SLASHEQ):
	return ici_str_ret("/=");
    case t_subtype(T_PERCENTEQ):
	return ici_str_ret("%=");
    case t_subtype(T_GRTGRTEQ):
	return ici_str_ret(">>=");
    case t_subtype(T_LESSLESSEQ):
	return ici_str_ret("<<=");
    case t_subtype(T_ANDEQ):
	return ici_str_ret("&=");
    case t_subtype(T_CARETEQ):
	return ici_str_ret("^=");
    case t_subtype(T_BAREQ):
	return ici_str_ret("|=");
    case t_subtype(T_2TILDEEQ):
	return ici_str_ret("~~=");
    case t_subtype(T_LESSEQGRT):
	return ici_str_ret("<=>");
    case t_subtype(T_COMMA):
	return ici_str_ret(",");
    }

    ici_error = "unrecognised binary operator";
    return 1;
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ, "expose", f_expose},
    {ICI_CF_OBJ, "sourcemarker", f_sourcemarker},
    {ICI_CF_OBJ, "operator", f_operator},
    {ICI_CF_OBJ, "binop", f_binop},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_vm_library_init(void)
{
#if DEBUG
    debug_out = fopen("/dev/tty", "w");
    if (debug_out)
	fprintf(debug_out, "vm module loading\n");
#endif
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "vm"))
        return NULL;
    if (init_ici_str())
        return NULL;
#if DEBUG
    if (debug_out)
	fprintf(debug_out, "vm module loaded\n");
#endif
    return ici_objof(ici_module_new(cfuncs));
}

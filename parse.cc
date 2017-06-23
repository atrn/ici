#define ICI_CORE
#include "fwd.h"
#include "parse.h"
#include "func.h"
#include "str.h"
#include "struct.h"
#include "null.h"
#include "src.h"
#include "buf.h"
#include "file.h"
#include "op.h"
#include "exec.h"
#include "re.h"
#include "set.h"
#include "mark.h"
#include "pc.h"
#include "src.h"
#include <errno.h>

namespace ici
{

/*
 * Some commonly used strings.
 */
static char     not_by[]        = "not followed by";
static char     an_expression[] = "an expression";

/*
 * A few forward definitions...
 */
static int      compound_statement(parse *, ici_struct *);
static int      expr(parse *, expr_t **, int);
static int      const_expression(parse *, object **, int);
static int      statement(parse *, array *, ici_struct *, const char *, int);

#define DISASSEMBLE   0
#if DISASSEMBLE
static char *
opname(ici_op_t *op)
{
    switch (op->op_ecode)
    {
    case ICI_OP_OTHER: return "ICI_OP_OTHER";
    case ICI_OP_CALL: return "ICI_OP_CALL";
    case ICI_OP_NAMELVALUE: return "ICI_OP_NAMELVALUE";
    case ICI_OP_DOT: return "ICI_OP_DOT";
    case ICI_OP_DOTKEEP: return "ICI_OP_DOTKEEP";
    case ICI_OP_DOTRKEEP: return "ICI_OP_DOTRKEEP";
    case ICI_OP_ASSIGN: return "ICI_OP_ASSIGN";
    case ICI_OP_ASSIGN_TO_NAME: return "ICI_OP_ASSIGN_TO_NAME";
    case ICI_OP_ASSIGNLOCAL: return "ICI_OP_ASSIGNLOCAL";
    case ICI_OP_EXEC: return "ICI_OP_EXEC";
    case ICI_OP_LOOP: return "ICI_OP_LOOP";
    case ICI_OP_REWIND: return "ICI_OP_REWIND";
    case ICI_OP_ENDCODE: return "ICI_OP_ENDCODE";
    case ICI_OP_IF: return "ICI_OP_IF";
    case ICI_OP_IFELSE: return "ICI_OP_IFELSE";
    case ICI_OP_IFNOTBREAK: return "ICI_OP_IFNOTBREAK";
    case ICI_OP_IFBREAK: return "ICI_OP_IFBREAK";
    case ICI_OP_BREAK: return "ICI_OP_BREAK";
    case ICI_OP_QUOTE: return "ICI_OP_QUOTE";
    case ICI_OP_BINOP: return "ICI_OP_BINOP";
    case ICI_OP_AT: return "ICI_OP_AT";
    case ICI_OP_SWAP: return "ICI_OP_SWAP";
    case ICI_OP_BINOP_FOR_TEMP: return "ICI_OP_BINOP_FOR_TEMP";
    case ICI_OP_AGGR_KEY_CALL: return "ICI_OP_AGGR_KEY_CALL";
    case ICI_OP_COLON: return "ICI_OP_COLON";
    case ICI_OP_COLONCARET: return "ICI_OP_COLONCARET";
    case ICI_OP_METHOD_CALL: return "ICI_OP_METHOD_CALL";
    case ICI_OP_SUPER_CALL: return "ICI_OP_SUPER_CALL";
    case ICI_OP_ASSIGNLOCALVAR: return "ICI_OP_ASSIGNLOCALVAR";
    case ICI_OP_CRITSECT: return "ICI_OP_CRITSECT";
    case ICI_OP_WAITFOR: return "ICI_OP_WAITFOR";
    case ICI_OP_POP: return "ICI_OP_POP";
    case ICI_OP_CONTINUE: return "ICI_OP_CONTINUE";
    case ICI_OP_LOOPER: return "ICI_OP_LOOPER";
    case ICI_OP_ANDAND: return "ICI_OP_ANDAND";
    case ICI_OP_SWITCH: return "ICI_OP_SWITCH";
    case ICI_OP_SWITCHER: return "ICI_OP_SWITCHER";
    default: return "op by function";
    }
}

static void
disassemble(int indent, array *a)
{
    object           **e;
    char                n1[30];
    int                 i;

    for (i = 0, e = a->a_bot; e < a->a_top; ++i, ++e)
    {
        printf("%*d: ", indent, i);
        if (issrc(*e))
        {
            printf("%s, %d\n", srcof(*e)->s_filename->s_chars, srcof(*e)->s_lineno);
        }
        else if (ici_isop(*e))
        {
            printf("%s %d\n", opname(ici_opof(*e)), ici_opof(*e)->op_code);
        }
        else
        {
            printf("%s\n", ici_objname(n1, *e));
        }
        if (isarray(*e))
        {
            disassemble(indent + 4, arrayof(*e));
        }
    }
}
#endif

/*
 * In general, parseing functions return -1 on error (and set the global
 * error string), 0 if they encountered an early head symbol conflict (and
 * the parse stream has not been disturbed), and 1 if they actually got
 * what they were looking for.
 */

/*
 * 'this' is a convenience macro. Routines in this file conventionally use
 * the variable name 'p' for the pointer the current parse structure. Given
 * that, 'this' is the last token fetched. That is, the current head symbol.
 */
#define this    p->p_got.t_what

/*
 * next(p, a) and reject(p) are the basic token fetching (and rejecting)
 * functions (or macros). See ici_lex() for the meanins of the 'a'. 'p' is a
 * pointer to the subject parse sructure.
 */
#if defined ICI_INLINE_PARSER_CODE

#define next(p, a)  (p->p_ungot.t_what != T_NONE                        \
                     ? (p->p_got=p->p_ungot, p->p_ungot.t_what=T_NONE, this) \
                     : ici_lex(p, a))

#define reject(p) (p->p_ungot = p->p_got, this = T_NONE)

#else

static int
next(parse *p, array *a)
{
    if (p->p_ungot.t_what != T_NONE)
    {
        p->p_got = p->p_ungot;
        p->p_ungot.t_what = T_NONE;
        return this;
    }
    return ici_lex(p, a);
}

static void
reject(parse *p)
{
    p->p_ungot = p->p_got;
    this = T_NONE;
}
#endif

static int
not_followed_by(const char *a, const char *b)
{
    ici_set_error("\"%s\" %s %s", a, not_by, b);
    return -1;
}

static int
not_allowed(const char *what)
{
    ici_set_error("%s outside of %sable statement", what, what);
    return -1;
}

static void
increment_break_depth(parse *p)
{
    ++p->p_break_depth;
}

static void
decrement_break_depth(parse *p)
{
    --p->p_break_depth;
}

static void
increment_break_continue_depth(parse *p)
{
    ++p->p_break_depth;
    ++p->p_continue_depth;
}

static void
decrement_break_continue_depth(parse *p)
{
    --p->p_break_depth;
    --p->p_continue_depth;
}

/*
 * Returns a non-decref atomic array of identifiers parsed from a comma
 * seperated list, or NULL on error.  The array may be empty.
 */
static array *
ident_list(parse *p)
{
    array *a;

    if ((a = ici_array_new(0)) == NULL)
    {
        return NULL;
    }
    for (;;)
    {
        if (next(p, NULL) != T_NAME)
        {
            reject(p);
            return a;
        }
        if (a->stk_push_chk())
        {
            goto fail;
        }
        *a->a_top = p->p_got.t_obj;
        this = T_NONE; /* Take ownership of name. */
        (*a->a_top)->decref();
        ++a->a_top;
        if (next(p, NULL) != T_COMMA)
        {
            reject(p);
            return arrayof(ici_atom(a, 1));
        }
    }

 fail:
    a->decref();
    return NULL;
}

/*
 * The parse stream may be positioned just before the on-round of the
 * argument list of a function. Try to parse the function.
 *
 * Return -1, 0 or 1, usual conventions.  On success, returns a parsed
 * non-decref function in parse.p_got.t_obj.
 */
static int
function(parse *p, str *name)
{
    array *a;
    func  *f;
    func  *saved_func;
    object **fp;

    a = NULL;
    f = NULL;
    if (next(p, NULL) != T_ONROUND)
    {
        reject(p);
        return 0;
    }
    if ((a = ident_list(p)) == NULL)
    {
        return -1;
    }
    saved_func = p->p_func;
    if (next(p, NULL) != T_OFFROUND)
    {
        reject(p);
        not_followed_by("ident ( [args]", "\")\"");
        goto fail;
    }
    if ((f = ici_new_func()) == NULL)
    {
        goto fail;
    }
    if ((f->f_autos = ici_struct_new()) == NULL)
    {
        goto fail;
    }
    f->f_autos->decref();
    if (ici_assign(f->f_autos, SS(_func_), f))
    {
        goto fail;
    }
    for (fp = a->a_base; fp < a->a_top; ++fp)
    {
        if (ici_assign(f->f_autos, *fp, ici_null))
            goto fail;
    }
    ici_assign(f->f_autos, SS(vargs), ici_null);
    f->f_autos->o_super = ici_objwsupof(ici_vs.a_top[-1])->o_super;
    p->p_func = f;
    f->f_args = a;
    a->decref();
    a = NULL;
    f->f_name = name;
    switch (compound_statement(p, NULL))
    {
    case 0: not_followed_by("ident ( [args] )", "\"{\"");
    case -1: goto fail;
    }
    f->f_code = arrayof(p->p_got.t_obj);
    f->f_code->decref();
    if (f->f_code->a_top[-1] == &ici_o_end)
    {
        --f->f_code->a_top;
    }
    if (f->f_code->stk_push_chk(3))
    {
        goto fail;
    }
    *f->f_code->a_top++ = ici_null;
    *f->f_code->a_top++ = &ici_o_return;
    *f->f_code->a_top++ = &ici_o_end;
#   if DISASSEMBLE
    printf("%s()\n", name == NULL ? "?" : name->s_chars);
    disassemble(4, f->f_code);
#   endif
    f->f_autos = ici_structof(ici_atom(f->f_autos, 2));
    p->p_got.t_obj = ici_atom(f, 1);
    p->p_func = saved_func;
    return 1;

 fail:
    if (a != NULL)
    {
        a->decref();
    }
    if (f != NULL)
    {
        f->decref();
    }
    p->p_func = saved_func;
    return -1;
}

/*
 * ows is the struct (or whatever) the idents are going into.
 */
static int
data_def(parse *p, objwsup *ows)
{
    object   *o;     /* The value it is initialised with. */
    object   *n;     /* The name. */
    int         wasfunc;
    int         hasinit;

    n = NULL;
    o = NULL;
    wasfunc = 0;
    /*
     * Work through the list of identifiers being declared.
     */
    for (;;)
    {
        if (next(p, NULL) != T_NAME)
        {
            reject(p);
            ici_set_error("syntax error in variable definition");
            goto fail;
        }
        n = p->p_got.t_obj;
        this = T_NONE; /* Take ownership of name. */
        /*
         * Gather any initialisation or function.
         */
        hasinit = 0;
        switch (next(p, NULL))
        {
        case T_EQ:
            switch (const_expression(p, &o, T_COMMA))
            {
            case 0: not_followed_by("ident =", an_expression);
            case -1: goto fail;
            }
            hasinit = 1;
            break;

        case T_ONROUND:
            reject(p);
            if (function(p, ici_stringof(n)) < 0)
            {
                goto fail;
            }
            o = p->p_got.t_obj;
            wasfunc = 1;
            hasinit = 1;
            break;

        default:
            o = ici_null;
            o->incref();
            reject(p);
        }

        /*
         * Assign to the new variable if it doesn't appear to exist
         * or has an explicit initialisation.
         */
        if (hasinit || ici_fetch_base(ows, n) == ici_null)
        {
            if (ici_assign_base(ows, n, o))
            {
                goto fail;
            }
        }
        n->decref();
        n = NULL;
        o->decref();
        o = NULL;

        if (wasfunc)
        {
            return 1;
        }

        switch (next(p, NULL))
        {
        case T_COMMA: continue;
        case T_SEMICOLON: return 1;
        }
        reject(p);
        ici_set_error("variable definition not followed by \";\" or \",\"");
        goto fail;
    }

 fail:
    if (n != NULL)
    {
        n->decref();
    }
    if (o != NULL)
    {
        o->decref();
    }
    return -1;
}

static int
compound_statement(parse *p, ici_struct *sw)
{
    array *a;

    a = NULL;
    if (next(p, NULL) != T_ONCURLY)
    {
        reject(p);
        return 0;
    }
    if ((a = ici_array_new(0)) == NULL)
    {
        goto fail;
    }
    for (;;)
    {
        switch (statement(p, a, sw, NULL, 0))
        {
        case -1: goto fail;
        case 1: continue;
        }
        break;
    }
    if (next(p, a) != T_OFFCURLY)
    {
        reject(p);
        ici_set_error("badly formed statement");
        goto fail;
    }
    /*
     *  Drop any trailing source marker.
     */
    if (a->a_top > a->a_bot && issrc(a->a_top[-1]))
    {
        --a->a_top;
    }

    if (a->stk_push_chk())
    {
        goto fail;
    }
    *a->a_top++ = &ici_o_end;
    p->p_got.t_obj = a;
    return 1;

 fail:
    if (a != NULL)
    {
        a->decref();
    }
    return -1;
}

/*
 * Free an exprssesion tree and decref all the objects that it references.
 */
static void
free_expr(expr_t *e)
{
    expr_t      *e1;

    while (e != NULL)
    {
        if (e->e_arg[1] != NULL)
        {
            free_expr(e->e_arg[1]);
        }
        if (e->e_obj != NULL)
        {
            e->e_obj->decref();
        }
        e1 = e;
        e = e->e_arg[0];
        ici_tfree(e1, expr_t);
    }
}

/*
 * We have just got and accepted the '('. Now get the rest up to and
 * including the ')'.
 */
static int
bracketed_expr(parse *p, expr_t **ep)
{
    switch (expr(p, ep, T_NONE))
    {
    case 0: not_followed_by("(", an_expression);
    case -1: return -1;
    }
    if (next(p, NULL) != T_OFFROUND)
    {
        reject(p);
        return not_followed_by("( expr", "\")\"");
    }
    return 1;
}

/*
 * Parse a primaryexpression in the parse context 'p' and store the expression
 * tree of 'expr_t' type nodes under the pointer indicated by 'ep'. Usual
 * parseing return conventions (see comment near start of file). See the
 * comment on expr() for the meaning of exclude.
 */
static int
primary(parse *p, expr_t **ep, int exclude)
{
    expr_t          *e;
    array     *a;
    ici_struct    *d;
    ici_set_t       *s;
    object       *n;
    object       *o;
    const char      *token_name = 0;
    int             wasfunc;
    object       *name;
    int             token;

    *ep = NULL;
    if ((e = ici_talloc(expr_t)) == NULL)
    {
        return -1;
    }
    e->e_arg[0] = NULL;
    e->e_arg[1] = NULL;
    e->e_obj = NULL;
    switch (next(p, NULL))
    {
    case T_INT:
        e->e_what = T_INT;
        if ((e->e_obj = ici_int_new(p->p_got.t_int)) == NULL)
        {
            goto fail;
        }
        break;

    case T_FLOAT:
        e->e_what = T_FLOAT;
        if ((e->e_obj = ici_float_new(p->p_got.t_float)) == NULL)
        {
            goto fail;
        }
        break;

    case T_REGEXP:
        e->e_what = T_CONST;
        token = T_REGEXP;
        goto gather_string_or_re;

    case T_STRING:
        e->e_what = T_STRING;
        token = T_STRING;
    gather_string_or_re:
        this = T_NONE; /* Take ownership of obj. */
        o = p->p_got.t_obj;
        while (next(p, NULL) == token || (token == T_REGEXP && this == T_STRING))
        {
            int        i;

            i = ici_stringof(p->p_got.t_obj)->s_nchars;
            if (chkbuf(ici_stringof(o)->s_nchars + i + 1))
            {
                goto fail;
            }
            memcpy(buf, ici_stringof(o)->s_chars, ici_stringof(o)->s_nchars);
            memcpy
            (
                buf + ici_stringof(o)->s_nchars,
                ici_stringof(p->p_got.t_obj)->s_chars,
                i
            );
            i += ici_stringof(o)->s_nchars;
            o->decref();
            this = T_NONE; /* Take ownership of obj. */
            p->p_got.t_obj->decref();
            if ((o = ici_str_new(buf, i)) == NULL)
            {
                goto fail;
            }
            this = T_NONE;
        }
        reject(p);
        if (token == T_REGEXP)
        {
            e->e_obj = ici_regexp_new(ici_stringof(o), 0);
            o->decref();
            if (e->e_obj == NULL)
            {
                goto fail;
            }
        }
        else
        {
            e->e_obj = o;
        }
        break;

    case T_NAME:
        this = T_NONE; /* Take ownership of name. */
        if (p->p_got.t_obj == SS(_NULL_))
        {
            e->e_what = T_NULL;
            p->p_got.t_obj->decref();
            break;
        }
        if (p->p_got.t_obj == SS(_false_))
        {
            e->e_what = T_INT;
            p->p_got.t_obj->decref();
            e->e_obj = ici_zero;
            e->e_obj->incref();
            break;
        }
        if (p->p_got.t_obj == SS(_true_))
        {
            e->e_what = T_INT;
            p->p_got.t_obj->decref();
            e->e_obj = ici_one;
            e->e_obj->incref();
            break;
        }
        e->e_what = T_NAME;
        e->e_obj = p->p_got.t_obj;
        break;

    case T_ONROUND:
        ici_tfree(e, expr_t);
        e = NULL;
        if (bracketed_expr(p, &e) < 1)
        {
            goto fail;
        }
        break;

    case T_ONSQUARE:
        if (next(p, NULL) != T_NAME)
        {
            reject(p);
            not_followed_by("[", "an identifier");
            goto fail;
        }
        this = T_NONE; /* Take ownership of name. */
        if (p->p_got.t_obj == SS(array))
        {
            p->p_got.t_obj->decref();
            if ((a = ici_array_new(0)) == NULL)
            {
                goto fail;
            }
            for (;;)
            {
                switch (const_expression(p, &o, T_COMMA))
                {
                case -1:
                    a->decref();
                    goto fail;

                case 1:
                    if (a->stk_push_chk())
                    {
                        a->decref();
                        goto fail;
                    }
                    *a->a_top++ = o;
                    o->decref();
                    if (next(p, NULL) == T_COMMA)
                    {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, NULL) != T_OFFSQUARE)
            {
                reject(p);
                a->decref();
                not_followed_by("[array expr, expr ...", "\",\" or \"]\"");
                goto fail;
            }
            e->e_what = T_CONST;
            e->e_obj = a;
        }
        else if
        (
            (name = p->p_got.t_obj) == SS(class)
            ||
            name == SS(struct)
            ||
            name == SS(module)
        )
        {
            ici_struct    *super;

            p->p_got.t_obj->decref();
            d = NULL;
            super = NULL;
            if (next(p, NULL) == T_COLON || this == T_EQ)
            {
                int     is_eq;
                char    n[30];

                is_eq = this == T_EQ;
                switch (const_expression(p, &o, T_COMMA))
                {
                case 0:
                    sprintf(n, "[%s %c", ici_stringof(name)->s_chars, is_eq ? '=' : ':');
                    not_followed_by(n, an_expression);
                case -1:
                    goto fail;
                }
                if (!ici_hassuper(o))
                {
                    ici_set_error("attempt to do [%s %c %s",
                                  ici_stringof(name)->s_chars,
                                  is_eq ? '=' : ':',
                                  ici_objname(n, o));
                    o->decref();
                    goto fail;
                }
                if (is_eq)
                {
                    d = ici_structof(o);
                }
                else
                {
                    super = ici_structof(o);
                }
                switch (next(p, NULL))
                {
                case T_OFFSQUARE:
                    reject(p);
                case T_COMMA:
                    break;

                default:
                    reject(p);
                    if (super != NULL)
                    {
                        super->decref();
                    }
                    if (d != NULL)
                    {
                        o->decref();
                    }
                    sprintf(n, "[%s %c expr", ici_stringof(name)->s_chars, is_eq ? '=' : ':');
                    not_followed_by(n, "\",\" or \"]\"");
                    goto fail;
                }
            }
            else
            {
                reject(p);
            }
            if (d == NULL)
            {
                if ((d = ici_struct_new()) == NULL)
                {
                    goto fail;
                }
                if (super == NULL)
                {
                    if (name != SS(struct))
                    {
                        d->o_super = ici_objwsupof(ici_vs.a_top[-1])->o_super;
                    }
                }
                else
                {
                    d->o_super = ici_objwsupof(super);
                    super->decref();
                }
            }

            if (name == SS(module))
            {
                ici_struct    *autos;

                if ((autos = ici_struct_new()) == NULL)
                {
                    d->decref();
                    goto fail;
                }
                autos->o_super = ici_objwsupof(d);
                *ici_vs.a_top++ = autos;
                autos->decref();
                ++p->p_module_depth;
                o = ici_evaluate(p, 0);
                --p->p_module_depth;
                --ici_vs.a_top;
                if (o == NULL)
                {
                    d->decref();
                    goto fail;
                }
                o->decref();
                e->e_what = T_CONST;
                e->e_obj = d;
                break;
            }
            if (name == SS(class))
            {
                ici_struct    *autos;

                /*
                 * A class definition operates within the scope context of
                 * the class. Create autos with the new struct as the super.
                 */
                if ((autos = ici_struct_new()) == NULL)
                {
                    d->decref();
                    goto fail;
                }
                autos->o_super = ici_objwsupof(d);
                if (ici_vs.stk_push_chk(80)) /* ### Formalise */
                {
                    d->decref();
                    goto fail;
                }
                *ici_vs.a_top++ = autos;
                autos->decref();
            }
            for (;;)
            {
                switch (next(p, NULL))
                {
                case T_OFFSQUARE:
                    break;

                case T_ONROUND:
                    switch (const_expression(p, &o, T_NONE))
                    {
                    case 0: not_followed_by("[struct ... (", an_expression);
                    case -1: d->decref(); goto fail;
                    }
                    if (next(p, NULL) != T_OFFROUND)
                    {
                        reject(p);
                        not_followed_by("[struct ... (expr", "\")\"");
                        d->decref();
                        goto fail;
                    }
                    n = o;
                    goto gotkey;

                case T_NAME:
                    n = p->p_got.t_obj;
                    this = T_NONE; /* Take ownership of name. */
                gotkey:
                    wasfunc = 0;
                    if (next(p, NULL) == T_ONROUND)
                    {
                        reject(p);
                        if (function(p, ici_stringof(n)) < 0)
                        {
                            d->decref();
                            goto fail;
                        }
                        o = p->p_got.t_obj;
                        wasfunc = 1;
                    }
                    else if (this == T_EQ)
                    {
                        switch (const_expression(p, &o, T_COMMA))
                        {
                        case 0: not_followed_by("[struct ... ident =", an_expression);
                        case -1: d->decref(); goto fail;
                        }
                    }
                    else if (this == T_COMMA || this == T_OFFSQUARE)
                    {
                        reject(p);
                        o = ici_null;
                        o->incref();
                    }
                    else
                    {
                        reject(p);
                        not_followed_by("[struct ... key", "\"=\", \"(\", \",\" or \"]\"");
                        d->decref();
                        n->decref();
                        goto fail;
                    }
                    if (ici_assign_base(d, n, o))
                    {
                        goto fail;
                    }
                    n->decref();
                    o->decref();
                    switch (next(p, NULL))
                    {
                    case T_OFFSQUARE:
                        reject(p);
                    case T_COMMA:
                        continue;

                    default:
                        if (wasfunc)
                        {
                            reject(p);
                            continue;
                        }
                    }
                    reject(p);
                    not_followed_by("[struct ... key = expr", "\",\" or \"]\"");
                    d->decref();
                    goto fail;

                default:
                    reject(p);
                    not_followed_by("[struct ...", "an initialiser");
                    d->decref();
                    goto fail;
                }
                break;
            }
            if (name == SS(class))
            {
                /*
                 * That was a class definition. Restore the scope context.
                 */
                --ici_vs.a_top;
            }
            e->e_what = T_CONST;
            e->e_obj = d;
        }
        else if (p->p_got.t_obj == SS(set))
        {
            p->p_got.t_obj->decref();
            if ((s = ici_set_new()) == NULL)
            {
                goto fail;
            }
            for (;;)
            {
                switch (const_expression(p, &o, T_COMMA))
                {
                case -1:
                    s->decref();
                    goto fail;

                case 1:
                    if (ici_assign(s, o, ici_one))
                    {
                        s->decref();
                        goto fail;
                    }
                    o->decref();
                    if (next(p, NULL) == T_COMMA)
                    {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, NULL) != T_OFFSQUARE)
            {
                reject(p);
                s->decref();
                not_followed_by("[set expr, expr ...", "\"]\"");
                goto fail;
            }
            e->e_what = T_CONST;
            e->e_obj = s;
        }
        else if (p->p_got.t_obj == SS(func))
        {
            p->p_got.t_obj->decref();
            switch (function(p, SS(empty_string)))
            {
            case 0: not_followed_by("[func", "function body");
            case -1:
                goto fail;
            }
            e->e_what = T_CONST;
            e->e_obj = p->p_got.t_obj;
            if (next(p, NULL) != T_OFFSQUARE)
            {
                reject(p);
                not_followed_by("[func function-body ", "\"]\"");
                goto fail;
            }
        }
        else
        {
            str    *s;     /* Name after the on-square . */
            file   *f;     /* The parse file. */
            object *c;     /* The callable parser function. */

            f = NULL;
            n = NULL;
            c = NULL;
            s = ici_stringof(p->p_got.t_obj);
            if ((o = ici_eval(s)) == NULL)
            {
                goto fail_user_parse;
            }
            if (o->can_call())
            {
                c = o;
                o = NULL;
            }
            else
            {
                if ((c = ici_fetch(o, SS(parser))) == NULL)
                {
                    goto fail_user_parse;
                }
            }
            f = ici_file_new(p, parse_ftype, p->p_file->f_name, p);
            if (f == NULL)
                goto fail_user_parse;
            c->incref();
            if (ici_func(c, "o=o", &n, f))
            {
                goto fail_user_parse;
            }
            e->e_what = T_CONST;
            e->e_obj = n;
            s->decref();
            if (o != NULL)
            {
                o->decref();
            }
            f->decref();
            c->decref();
            if (next(p, NULL) != T_OFFSQUARE)
            {
                reject(p);
                not_followed_by("[name ... ", "\"]\"");
                goto fail;
            }
            break;

        fail_user_parse:
            s->decref();
            if (o != NULL)
            {
                o->decref();
            }
            if (f != NULL)
            {
                f->decref();
            }
            if (c != NULL)
            {
                c->decref();
            }
            goto fail;
        }
        break;

    default:
        reject(p);
        ici_tfree(e, expr_t);
        return 0;
    }
    *ep = e;
    e = NULL;
    for (;;)
    {
        int     oldthis;

        switch (next(p, NULL))
        {
        case T_ONSQUARE:
            if ((e = ici_talloc(expr_t)) == NULL)
            {
                goto fail;
            }
            e->e_what = T_ONSQUARE;
            e->e_arg[0] = *ep;
            e->e_arg[1] = NULL;
            e->e_obj = NULL;
            *ep = e;
            e = NULL;
            switch (expr(p, &(*ep)->e_arg[1], T_NONE))
            {
            case 0: not_followed_by("[", an_expression);
            case -1: goto fail;
            }
            if (next(p, NULL) != T_OFFSQUARE)
            {
                reject(p);
                not_followed_by("[ expr", "\"]\"");
                goto fail;
            }
            break;

        case T_COLON:
        case T_COLONCARET:
        case T_PTR:
        case T_DOT:
        case T_AT:
            if (this == exclude)
            {
                reject(p);
                return 1;
            }
            if ((e = ici_talloc(expr_t)) == NULL)
            {
                goto fail;
            }
            if ((oldthis = this) == T_AT)
            {
                e->e_what = T_BINAT;
            }
            else if (oldthis == T_COLON)
            {
                e->e_what = T_PRIMARYCOLON;
            }
            else
            {
                e->e_what = this;
            }
            e->e_arg[0] = *ep;
            e->e_arg[1] = NULL;
            e->e_obj = NULL;
            *ep = e;
            e = NULL;
            switch (next(p, NULL))
            {
            case T_NAME:
                this = T_NONE; /* Take ownership of name. */
                if ((e = ici_talloc(expr_t)) == NULL)
                {
                    goto fail;
                }
                e->e_what = T_STRING;
                e->e_arg[0] = NULL;
                e->e_arg[1] = NULL;
                e->e_obj = NULL;
                e->e_obj = p->p_got.t_obj;
                (*ep)->e_arg[1] = e;
                e = NULL;
                break;

            case T_ONROUND:
                if (bracketed_expr(p, &(*ep)->e_arg[1]) < 1)
                {
                    goto fail;
                }
                break;

            default:
                reject(p);
                switch (oldthis)
                {
                case T_COLON:      token_name = ":"; break;
                case T_COLONCARET: token_name = ":^"; break;
                case T_PTR:        token_name = "->"; break;
                case T_DOT:        token_name = "."; break;
                case T_AT:         token_name = "@"; break;
                default:           assert(0);
                }
                not_followed_by(token_name, "an identifier or \"(\"");
                goto fail;
            }
            break;

        case T_ONROUND: /* Function call. */
            if ((e = ici_talloc(expr_t)) == NULL)
            {
                goto fail;
            }
            e->e_what = T_ONROUND;
            e->e_arg[0] = *ep;
            e->e_arg[1] = NULL;
            e->e_obj = NULL;
            *ep = e;
            e = NULL;
            for (;;)
            {
                expr_t  *e1;

                e1 = NULL;
                switch (expr(p, &e1, T_COMMA))
                {
                case -1:
                    goto fail;

                case 1:
                    if ((e = ici_talloc(expr_t)) == NULL)
                    {
                        goto fail;
                    }
                    e->e_arg[1] = (*ep)->e_arg[1];
                    (*ep)->e_arg[1] = e;
                    e->e_what = T_COMMA;
                    e->e_arg[0] = e1;
                    e->e_obj = NULL;
                    e = NULL;
                    if (next(p, NULL) == T_COMMA)
                    {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, NULL) != T_OFFROUND)
            {
                reject(p);
                ici_set_error("error in function call arguments");
                goto fail;
            }
            if (next(p, NULL) == T_ONCURLY)
            {
                /*
                 * Gratuitous check to get a better error message.
                 */
                ici_set_error("function definition without a storage class");
                goto fail;
            }
            reject(p);
            break;


        default:
            reject(p);
            return 1;
        }
    }

 fail:
    if (e != NULL)
    {
        if (e->e_obj != NULL)
        {
            e->e_obj->decref();
        }
        ici_tfree(e, expr_t);
    }
    free_expr(*ep);
    *ep = NULL;
    return -1;
}

/*
 * Parse a sub-expression consisting or a sequence of unary operators round
 * a primary (a factor) in the parse context 'p' and store the expression
 * tree of 'expr_t' type nodes under the pointer indicated by 'ep'. Usual
 * parseing return conventions (see comment near start of file). See the
 * comment on expr() for the meaning of exclude.
 */
static int
unary(parse *p, expr_t **ep, int exclude)
{
    expr_t      *e;
    int         what;

    switch (next(p, NULL))
    {
    case T_ASTERIX:
    case T_AND:
    case T_MINUS:
    case T_PLUS:
    case T_EXCLAM:
    case T_TILDE:
    case T_PLUSPLUS:
    case T_MINUSMINUS:
    case T_AT:
    case T_DOLLAR:
        what = this;
        switch (unary(p, ep, exclude))
        {
        case 0: ici_set_error("badly formed expression");
        case -1: return -1;
        }
        if ((e = ici_talloc(expr_t)) == NULL)
        {
            return -1;
        }
        e->e_what = what;
        e->e_arg[0] = *ep;
        e->e_arg[1] = NULL;
        e->e_obj = NULL;
        *ep = e;
        break;

    default:
        reject(p);
        switch (primary(p, ep, exclude))
        {
        case 0: return 0;
        case -1: return -1;
        }
    }
    switch (next(p, NULL))
    {
    case T_PLUSPLUS:
    case T_MINUSMINUS:
        if ((e = ici_talloc(expr_t)) == NULL)
        {
            return -1;
        }
        e->e_what = this;
        e->e_arg[0] = NULL;
        e->e_arg[1] = *ep;
        e->e_obj = NULL;
        *ep = e;
        break;

    default:
        reject(p);
        break;
    }
    return 1;
}

/*
 * Parse an expression in the parse context 'p' and store the expression
 * tree of 'expr_t' type nodes under the pointer indicated by 'ep'. Usual
 * parseing return conventions (see comment near start of file).
 *
 * exclude              A binop token that would normally be allowed
 *                      but because of the context this expression is
 *                      being parsed in, must be excluded. This is used
 *                      to exclude commas from argument lists and colons
 *                      from case labels and such like. The support is
 *                      not general. Only what is needed for comma and
 *                      colon.
 */
static int
expr(parse *p, expr_t **ep, int exclude)
{
    expr_t      *e;
    expr_t      **ebase;
    expr_t      *elimit;
    int         tp;
    int         r;
    int         in_quest_colon;

    /*
     * This expression parser is neither state stack based nor recursive
     * descent. It maintains an expression tree, and re-forms it each time
     * it finds a subsequent binary operator and following factor. In
     * practice this is probably faster than either the other two methods.
     * It handles all the precedence and right/left associativity and
     * the ? : operator correctly (at least according to ICI's definition
     * of ? :).
     */

    /*
     * Get the first factor.
     */
    if ((r = unary(p, ebase = ep, exclude)) <= 0)
    {
        return r;
    }
    elimit = *ebase;

    /*
     * While there is a following binary operator, merge it and the
     * following factor into the expression.
     */
    while (t_type(next(p, NULL)) == T_BINOP && this != exclude)
    {
        /*
         * Cause assignments to be right associative.
         */
        if ((tp = t_prec(this)) == t_prec(T_EQ))
        {
            --tp;
        }

        /*
         * Slide down the right hand side of the tree to find where this
         * operator binds.
         */
        in_quest_colon = this == T_QUESTION;
        for
        (
            ep = ebase;
            (e = *ep) != elimit && tp < t_prec(e->e_what);
            ep = &e->e_arg[1]
        )
        {
            if (e->e_what == T_QUESTION)
            {
                in_quest_colon = 1;
            }
        }

        /*
         * Allocate a new node and rebuild this bit with the new operator
         * and the following factor.
         */
        if ((e = ici_talloc(expr_t)) == NULL)
        {
            return -1;
        }
        e->e_what = this;
        e->e_arg[0] = *ep;
        e->e_arg[1] = NULL;
        e->e_obj = NULL;
        switch (unary(p, &e->e_arg[1], in_quest_colon ? T_COLON : exclude))
        {
        case 0:
            ici_set_error("\"expr %s\" %s %s", binop_name(t_subtype(e->e_what)), not_by, an_expression);
        case -1:
            ici_tfree(e, expr_t);
            return -1;
        }
        *ep = e;
        elimit = e->e_arg[1];
    }
    reject(p);
    return 1;
}

static int
expression(parse *p, array *a, int why, int exclude)
{
    expr_t      *e;

    e = NULL;
    switch (expr(p, &e, exclude))
    {
    case 0: return 0;
    case -1: goto fail;
    }
    if (ici_compile_expr(a, e, why))
    {
        goto fail;
    }
    free_expr(e);
    return 1;

 fail:
    free_expr(e);
    return -1;
}

/*
 * Parse and evaluate an expected "constant" (that is, parse time evaluated)
 * expression and store the result through po. The excluded binop (see comment
 * on expr() above) is often used to exclude comma operators.
 *
 * Usual parseing return conventions.
 */
static int
const_expression(parse *p, object **po, int exclude)
{
    expr_t      *e;
    array *a;
    int         ret;

    a = NULL;
    e = NULL;
    if ((ret = expr(p, &e, exclude)) <= 0)
    {
        return ret;
    }
    /*
     * If the expression is a data item that obviously requires no
     * further compilation and evaluation to arrive at its value,
     * we just use the value directly.
     */
    switch (e->e_what)
    {
    case T_NULL:
        *po = ici_null;
        goto simple;

    case T_INT:
    case T_FLOAT:
    case T_STRING:
    case T_CONST:
        *po = e->e_obj;
    simple:
        (*po)->incref();
        free_expr(e);
        return 1;
    }
    if ((a = ici_array_new(0)) == NULL)
    {
        goto fail;
    }
    if (ici_compile_expr(a, e, FOR_VALUE))
    {
        goto fail;
    }
    if (a->stk_push_chk())
    {
        goto fail;
    }
    *a->a_top++ = &ici_o_end;
    free_expr(e);
    e = NULL;
    if ((*po = ici_evaluate(a, 0)) == NULL)
    {
        goto fail;
    }
    a->decref();
    return 1;

 fail:
    if (a != NULL)
    {
        a->decref();
    }
    free_expr(e);
    return -1;
}

static int
xx_brac_expr_brac(parse *p, array *a, const char *xx)
{
    if (next(p, a) != T_ONROUND)
    {
        reject(p);
        sprintf(buf, "\"%s\" %s a \"(\"", xx, not_by);
        goto fail;
    }
    switch (expression(p, a, FOR_VALUE, T_NONE))
    {
    case 0:
        sprintf(buf, "\"%s (\" %s %s", xx, not_by, an_expression);
        goto fail;

    case -1:
        return -1;
    }
    if (next(p, a) != T_OFFROUND)
    {
        reject(p);
        sprintf(buf, "\"%s (expr\" %s \")\"", xx, not_by);
        goto fail;
    }
    return 1;

 fail:
    ici_set_error("%s", buf);
    return -1;
}

/*
 * a    Code array being appended to.
 * sw   Switch structure, else NULL.
 * m    Who needs it, else NULL.
 * endme If non-zero, put an ici_o_end at the end of the code array before
 *      returning.
 */
static int
statement(parse *p, array *a, ici_struct *sw, const char *m, int endme)
{
    array         *a1;
    array         *a2;
    expr_t              *e;
    ici_struct        *d;
    objwsup       *ows;
    object           *o;
    ici_int_t           *i;
    int                 stepz;

    switch (next(p, a))
    {
    case T_ONCURLY:
        for (;;)
        {
            switch (statement(p, a, NULL, NULL, 0))
            {
            case -1: return -1;
            case 1: continue;
            }
            break;
        }
        if (next(p, a) != T_OFFCURLY)
        {
            reject(p);
            ici_set_error("badly formed statement");
            return -1;
        }
        break;

    case T_SEMICOLON:
        break;

    case T_OFFCURLY: /* Just to prevent unecessary expression parseing. */
    case T_EOF:
    case T_ERROR:
        reject(p);
        goto none;

    case T_NAME:
        this = T_NONE; /* Assume we own the name. */
        if (p->p_got.t_obj == SS(export))
        {
            p->p_got.t_obj->decref();
            if
            (
                (ows = ici_objwsupof(ici_vs.a_top[-1])->o_super) == NULL
                ||
                (ows = ows->o_super) == NULL
            )
            {
                ici_set_error("global declaration, but no global variable scope");
                return -1;
            }
            goto decl;
        }
        if (p->p_got.t_obj == SS(local))
        {
            p->p_got.t_obj->decref();
            if ((ows = ici_objwsupof(ici_vs.a_top[-1])->o_super) == NULL)
            {
                ici_set_error("local declaration, but no local variable scope");
                return -1;
            }
            goto decl;
        }
        if (p->p_got.t_obj == SS(var))
        {
            p->p_got.t_obj->decref();
            if (p->p_func == NULL)
            {
                ows = ici_objwsupof(ici_vs.a_top[-1]);
            }
            else
            {
                ows = ici_objwsupof(p->p_func->f_autos);
            }
        decl:
            if (data_def(p, ows) == -1)
            {
                return -1;
            }
            break;
        }
        if (p->p_got.t_obj == SS(case))
        {
            p->p_got.t_obj->decref();
            if (sw == NULL)
            {
                ici_set_error("\"case\" not at top level of switch body");
                return -1;
            }
            switch (const_expression(p, &o, T_COLON))
            {
            case 0: not_followed_by("case", an_expression);
            case -1: return -1;
            }
            stepz = a->a_top - a->a_bot;
            if (stepz > 0 && issrc(a->a_top[-1]))
            {
                /*
                 * If the last thing in the code array is a source marker,
                 * make the case label jump to that. This is not only
                 * useful for correct line tracking, but essential, because
                 * if there is no further statement, the source marker will
                 * be trimmed off the array and the jump will land off the
                 * end.
                 */
                --stepz;
            }
            if ((i = ici_int_new((long)stepz)) == NULL)
            {
                o->decref();
                return -1;
            }
            if (ici_assign(sw, o, i))
            {
                i->decref();
                o->decref();
                return -1;
            }
            i->decref();
            o->decref();
            if (next(p, a) != T_COLON)
            {
                reject(p);
                return not_followed_by("case expr", "\":\"");
            }
            break;
        }
        if (p->p_got.t_obj == SS(default))
        {
            p->p_got.t_obj->decref();
            if (sw == NULL)
            {
                ici_set_error("\"default\" not at top level of switch body");
                return -1;
            }
            if (next(p, a) != T_COLON)
            {
                reject(p);
                return not_followed_by("default", "\":\"");
            }
            stepz = a->a_top - a->a_bot;
            if (stepz > 0 && issrc(a->a_top[-1]))
            {
                /*
                 * If the last thing in the code array is a source marker,
                 * make the default label jump to that. This is not only
                 * useful for correct line tracking, but essential, because
                 * if there is no further statement, the source marker will
                 * be trimmed off the array and the jump will land off the
                 * end.
                 */
                --stepz;
            }
            if ((i = ici_int_new((long)stepz)) == NULL)
            {
                return -1;
            }
            if (ici_assign(sw, &ici_o_mark, i))
            {
                i->decref();
                return -1;
            }
            i->decref();
            break;
        }
        if (p->p_got.t_obj == SS(if))
        {
            p->p_got.t_obj->decref();
            if (xx_brac_expr_brac(p, a, "if") != 1)
            {
                return -1;
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            if (statement(p, a1, NULL, "if (expr)", 1) == -1)
            {
                a1->decref();
                return -1;
            }
            a2 = NULL;
            /*
             * Don't pass any code array to next() on else clause to stop
             * spurious src marker.
             */
            if (next(p, NULL) == T_NAME && p->p_got.t_obj == SS(else))
            {
                this = T_NONE; /* Take ownership of name. */
                p->p_got.t_obj->decref();
                if ((a2 = ici_array_new(0)) == NULL)
                {
                    a1->decref();
                    return -1;
                }
                if (statement(p, a2, NULL, "if (expr) stmt else", 1) == -1)
                {
                    a1->decref();
                    a2->decref();
                    return -1;
                }
            }
            else
            {
                reject(p);
            }
            if (a->stk_push_chk(3))
            {
                a1->decref();
                if (a2 != NULL)
                {
                    a2->decref();
                }
                return -1;
            }
            if (a2 != NULL)
            {
                *a->a_top++ = &ici_o_ifelse;
                *a->a_top++ = a1;
                *a->a_top++ = a2;
                a2->decref();
            }
            else
            {
                *a->a_top++ = &ici_o_if;
                *a->a_top++ = a1;
            }
            a1->decref();
            break;
        }
        if (p->p_got.t_obj == SS(while))
        {
            p->p_got.t_obj->decref();
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            if (xx_brac_expr_brac(p, a1, "while") != 1)
            {
                a1->decref();
                return -1;
            }
            if (a1->stk_push_chk())
            {
                a1->decref();
                return -1;
            }
            *a1->a_top++ = &ici_o_ifnotbreak;
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, NULL, "while (expr)", 0);
                decrement_break_continue_depth(p);
                if (rc == -1)
                {
                    a1->decref();
                    return -1;
                }
            }
            if (a1->stk_push_chk())
            {
                a1->decref();
                return -1;
            }
            *a1->a_top++ = &ici_o_rewind;
            if (a->stk_push_chk(2))
            {
                a1->decref();
                return -1;
            }
            *a->a_top++ = &ici_o_loop;
            *a->a_top++ = a1;
            a1->decref();
            break;
        }
        if (p->p_got.t_obj == SS(do))
        {
            p->p_got.t_obj->decref();
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, NULL, "do", 0);
                decrement_break_continue_depth(p);
                if (rc == -1)
                {
                    a1->decref();
                    return -1;
                }
            }
            if (next(p, a1) != T_NAME || p->p_got.t_obj != SS(while))
            {
                reject(p);
                a1->decref();
                return not_followed_by("do statement", "\"while\"");
            }
            this = T_NONE; /* Take ownership of name. */
            p->p_got.t_obj->decref();
            if (next(p, NULL) != T_ONROUND)
            {
                reject(p);
                a1->decref();
                return not_followed_by("do statement while", "\"(\"");
            }
            switch (expression(p, a1, FOR_VALUE, T_NONE))
            {
            case 0: ici_set_error("syntax error");
            case -1: a1->decref(); return -1;
            }
            if (next(p, a1) != T_OFFROUND || next(p, NULL) != T_SEMICOLON)
            {
                reject(p);
                a1->decref();
                return not_followed_by("do statement while (expr", "\");\"");
            }
            if (a1->stk_push_chk(2))
            {
                a1->decref();
                return -1;
            }
            *a1->a_top++ = &ici_o_ifnotbreak;
            *a1->a_top++ = &ici_o_rewind;

            if (a->stk_push_chk(2))
            {
                a1->decref();
                return -1;
            }
            *a->a_top++ = &ici_o_loop;
            *a->a_top++ = a1;
            a1->decref();
            break;
        }
        if (p->p_got.t_obj == SS(forall))
        {
            int         rc;

            p->p_got.t_obj->decref();
            if (next(p, a) != T_ONROUND)
            {
                reject(p);
                return not_followed_by("forall", "\"(\"");
            }
            if ((rc = expression(p, a, FOR_LVALUE, T_COMMA)) == -1)
            {
                return -1;
            }
            if (rc == 0)
            {
                if (a->stk_push_chk(2))
                    return -1;
                *a->a_top++ = ici_null;
                *a->a_top++ = ici_null;
            }
            if (next(p, a) == T_COMMA)
            {
                if (expression(p, a, FOR_LVALUE, T_COMMA) == -1)
                {
                    return -1;
                }
                if (next(p, a) != T_NAME || p->p_got.t_obj != SS(in))
                {
                    reject(p);
                    return not_followed_by("forall (expr, expr", "\"in\"");
                }
                this = T_NONE; /* Take ownership of name. */
                p->p_got.t_obj->decref();
            }
            else
            {
                if (this != T_NAME || p->p_got.t_obj != SS(in))
                {
                    reject(p);
                    return not_followed_by("forall (expr", "\",\" or \"in\"");
                }
                this = T_NONE; /* Take ownership of name. */
                p->p_got.t_obj->decref();
                if (a->stk_push_chk(2))
                {
                    return -1;
                }
                *a->a_top++ = ici_null;
                *a->a_top++ = ici_null;
            }
            if (expression(p, a, FOR_VALUE, T_NONE) == -1)
            {
                return -1;
            }
            if (next(p, a) != T_OFFROUND)
            {
                reject(p);
                return not_followed_by("forall (expr [, expr] in expr", "\")\"");
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, NULL, "forall (expr [, expr] in expr)", 1);
                decrement_break_continue_depth(p);
                if (rc == -1)
                {
                    a1->decref();
                    return -1;
                }
            }
            if (a->stk_push_chk(2))
            {
                a1->decref();
                return -1;
            }
            *a->a_top++ = a1;
            a1->decref();
            if ((*a->a_top = ici_new_op(ici_op_forall, 0, 0)) == NULL)
            {
                return -1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
            break;

        }
        if (p->p_got.t_obj == SS(for))
        {
            p->p_got.t_obj->decref();
            if (next(p, a) != T_ONROUND)
            {
                reject(p);
                return not_followed_by("for", "\"(\"");
            }
            if (expression(p, a, FOR_EFFECT, T_NONE) == -1)
                return -1;
            if (next(p, a) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("for (expr", "\";\"");
            }

            /*
             * Get the condition expression, but don't generate code yet.
             */
            e = NULL;
            if (expr(p, &e, T_NONE) == -1)
            {
                return -1;
            }

            if (next(p, a) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("for (expr; expr", "\";\"");
            }

            /*
             * a1 is the body of the loop.  Get the step expression.
             */
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            if (expression(p, a1, FOR_EFFECT, T_NONE) == -1)
            {
                a1->decref();
                return -1;
            }
            stepz = a1->a_top - a1->a_base;

            if (e != NULL)
            {
                /*
                 * Now compile in the test expression.
                 */
                if (ici_compile_expr(a1, e, FOR_VALUE))
                {
                    free_expr(e);
                    a1->decref();
                    return -1;
                }
                free_expr(e);
                if (a1->stk_push_chk())
                {
                    a1->decref();
                    return -1;
                }
                *a1->a_top++ = &ici_o_ifnotbreak;
            }
            if (next(p, a1) != T_OFFROUND)
            {
                reject(p);
                a1->decref();
                return not_followed_by("for (expr; expr; expr", "\")\"");
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, NULL, "for (expr; expr; expr)", 0);
                decrement_break_continue_depth(p);
                if (rc == -1)
                {
                    a1->decref();
                    return -1;
                }
            }
            if (a1->stk_push_chk())
            {
                a1->decref();
                return -1;
            }
            *a1->a_top++ = &ici_o_rewind;
            if (a->stk_push_chk(2))
            {
                a1->decref();
                return -1;
            }
            *a->a_top++ = a1;
            a1->decref();
            if ((*a->a_top = ici_new_op(ici_op_for, 0, stepz)) == NULL)
            {
                return -1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
            break;
        }
        if (p->p_got.t_obj == SS(switch))
        {
            p->p_got.t_obj->decref();
            if (xx_brac_expr_brac(p, a, "switch") != 1)
            {
                return -1;
            }
            if ((d = ici_struct_new()) == NULL)
            {
                return -1;
            }
            {
                int rc;
                increment_break_depth(p);
                rc = compound_statement(p, d);
                decrement_break_depth(p);
                switch (rc)
                {
                case 0:
                    not_followed_by("switch (expr)", "a compound statement");
                case -1:
                    d->decref();
                    return -1;
                }
            }
            if (a->stk_push_chk(3))
            {
                d->decref();
                p->p_got.t_obj->decref();
                return -1;
            }
            *a->a_top++ = p->p_got.t_obj;
            p->p_got.t_obj->decref();
            *a->a_top++ = d;
            *a->a_top++ = &ici_o_switch;
            d->decref();
            break;
        }
        if (p->p_got.t_obj == SS(break))
        {
            p->p_got.t_obj->decref();
            if (p->p_break_depth == 0)
            {
                return not_allowed("break");
            }
            if (next(p, a) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("break", "\";\"");
            }
            if (a->stk_push_chk())
            {
                return -1;
            }
            *a->a_top++ = &ici_o_break;
            break;

        }
        if (p->p_got.t_obj == SS(continue))
        {
            p->p_got.t_obj->decref();
            if (p->p_continue_depth == 0)
            {
                return not_allowed("continue");
            }
            if (next(p, a) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("continue", "\";\"");
            }
            if (a->stk_push_chk())
            {
                return -1;
            }
            *a->a_top++ = &ici_o_continue;
            break;
        }
        if (p->p_got.t_obj == SS(return))
        {
            p->p_got.t_obj->decref();
            switch (expression(p, a, FOR_VALUE, T_NONE))
            {
            case -1: return -1;
            case 0:
                if (a->stk_push_chk())
		{
                    return -1;
		}
                if ((*a->a_top = ici_null) == NULL)
		{
                    return -1;
		}
                ++a->a_top;
            }
            if (next(p, a) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("return [expr]", "\";\"");
            }
            if (a->stk_push_chk())
	    {
                return -1;
	    }
            *a->a_top++ = &ici_o_return;
            break;
        }
        if (p->p_got.t_obj == SS(try))
        {
            p->p_got.t_obj->decref();
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            if (statement(p, a1, NULL, "try", 1) == -1)
            {
                a1->decref();
                return -1;
            }
            if (next(p, NULL) != T_NAME || p->p_got.t_obj != SS(onerror))
            {
                reject(p);
                a1->decref();
                return not_followed_by("try statement", "\"onerror\"");
            }
            this = T_NONE; /* Take ownership of name. */
            p->p_got.t_obj->decref();
            if ((a2 = ici_array_new(0)) == NULL)
            {
                return -1;
            }
            if (statement(p, a2, NULL, "try statement onerror", 1) == -1)
            {
                a1->decref();
                a2->decref();
                return -1;
            }
            if (a->stk_push_chk(3))
            {
                a1->decref();
                a2->decref();
                return -1;
            }
            *a->a_top++ = a1;
            *a->a_top++ = a2;
            *a->a_top++ = &ici_o_onerror;
            a1->decref();
            a2->decref();
            break;
        }
        if (p->p_got.t_obj == SS(critsect))
        {
            p->p_got.t_obj->decref();
            /*
             * Start a critical section with a new code array (a1) as
             * its subject. Into this code array we place the statement.
             */
            if (a->stk_push_chk(2))
            {
                return -1;
            }
            if ((a1 = ici_array_new(1)) == NULL)
            {
                return -1;
            }
            *a->a_top++ = a1;
            a1->decref();
            if (statement(p, a1, NULL, "critsect", 1) == -1)
            {
                return -1;
            }
            *a->a_top++ = &ici_o_critsect;
            break;
        }
        if (p->p_got.t_obj == SS(waitfor))
        {
            p->p_got.t_obj->decref();
            /*
             * Start a critical section with a new code array (a1) as
             * its subject. Into this code array we place a loop followed
             * by the statement. Thus the critical section will dominate
             * the entire waitfor statement. But the wait primitive
             * temporarily releases one level of critical section around
             * the actual wait call.
             */
            if (a->stk_push_chk(2))
            {
                return -1;
            }
            if ((a1 = ici_array_new(2)) == NULL)
            {
                return -1;
            }
            *a->a_top++ = a1;
            a1->decref();
            *a->a_top++ = &ici_o_critsect;
            /*
             * Start a new code array (a2) and establish it as the body of
             * a loop.
             */
            if (a1->stk_push_chk(2))
            {
                return -1;
            }
            if ((a2 = ici_array_new(2)) == NULL)
            {
                return -1;
            }
            *a1->a_top++ = &ici_o_loop;
            *a1->a_top++ = a2;
            a2->decref();
            /*
             * Into the new code array (a1, the body of the loop) we build:
             *     condition expression (for value)
             *     if-break operator
             *     wait object expression (for value)
             *     waitfor operator
             */
            if (next(p, a2) != T_ONROUND)
            {
                reject(p);
                return not_followed_by("waitfor", "\"(\"");
            }
            switch (expression(p, a2, FOR_VALUE, T_NONE))
            {
            case 0: not_followed_by("waitfor (", an_expression);
            case -1: return -1;
            }
            if (a2->stk_push_chk())
            {
                return -1;
            }
            *a2->a_top++ = &ici_o_ifbreak;
            if (next(p, a2) != T_SEMICOLON)
            {
                reject(p);
                return not_followed_by("waitfor (expr", "\";\"");
            }
            switch (expression(p, a2, FOR_VALUE, T_NONE))
            {
            case 0: not_followed_by("waitfor (expr;", an_expression);
            case -1: return -1;
            }
            if (a2->stk_push_chk(2))
            {
                return -1;
            }
            *a2->a_top++ = &ici_o_waitfor;
            *a2->a_top++ = &ici_o_rewind;
            /*
             * After we break out of the loop, we execute the statement,
             * but it is still on top of the critical section. After the
             * statement is executed, the execution engine will pop off
             * the critical section catcher.
             */
            if (next(p, a2) != T_OFFROUND)
            {
                reject(p);
                return not_followed_by("waitfor (expr; expr", "\")\"");
            }
            if (statement(p, a1, NULL, "waitfor (expr; expr)", 1) == -1)
            {
                return -1;
            }
            break;
        }
        this = T_NAME; /* Woops, we wan't that name afterall. */
    default:
        reject(p);
        switch (expression(p, a, FOR_EFFECT, T_NONE))
        {
        case 0: goto none;
        case -1: return -1;
        }
        switch (next(p, a))
        {
        case T_OFFCURLY:
        case T_EOF:
            reject(p);
            break;

        case T_SEMICOLON:
            break;

        default:
            reject(p);
            ici_set_error("badly formed expression, or missing \";\"");
            return -1;
        }
    }
    if (endme)
    {
        /*
         *  Drop any trailing source marker.
         */
        if (a->a_top > a->a_bot && issrc(a->a_top[-1]))
        {
            --a->a_top;
        }

        if (a->stk_push_chk())
        {
            return -1;
        }
        *a->a_top++ = &ici_o_end;
    }
    return 1;

 none:
    if (m != NULL)
    {
        ici_set_error("\"%s\" %s a reasonable statement", m, not_by);
        return -1;
    }
    return 0;
}

/*
 * Parse the given file 'f' in the given scope 's'. It is common to call
 * this function with 's' being 'ici_vs.a_top[-1]', that is, the current
 * scope.
 *
 * Returns non-zero on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_parse(file *f, objwsup *s)
{
    parse         *p;
    object           *o;

    if ((p = ici_new_parse(f)) == NULL)
    {
        return -1;
    }

    *ici_vs.a_top++ = s;
    if ((o = ici_evaluate(p, 0)) == NULL)
    {
        --ici_vs.a_top;
        p->decref();
        return -1;
    }
    --ici_vs.a_top;
    o->decref();
    p->decref();
    return 0;
}

/*
 * Parse a file as a new top-level module.  This function create new auto and
 * static scopes, and makes the current static scope the exern scope of the
 * new module.  This function takes a generic file-like stream.  The specific
 * stream is identified by 'file' and the stdio-like access functions required
 * to read it are contained in the structure pointed to by 'ftype'.  A name
 * for the module, for use in error messages, is supplied in 'mname'
 * (typically the name of the file).
 *
 * This function can be used when the source of data to be parsed is not a
 * real file, but some other source like a resource.
 *
 * The file is closed prior to a successful return, but not a failure.
 *
 * Return 0 if ok, else -1, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_parse_file(const char *mname, char *fileo, ftype *ftype)
{
    objwsup       *s;     /* Statics. */
    objwsup       *a;     /* Autos. */
    file          *f;

    a = NULL;
    f = NULL;
    if ((f = ici_file_new(fileo, ftype, ici_str_get_nul_term(mname), NULL)) == NULL)
    {
        goto fail;
    }

    if ((a = ici_objwsupof(ici_struct_new())) == NULL)
    {
        goto fail;
    }
    if (ici_assign(a, SS(_file_), f->f_name))
    {
        goto fail;
    }
    if ((a->o_super = s = ici_objwsupof(ici_struct_new())) == NULL)
    {
        goto fail;
    }
    s->decref();
    s->o_super = ici_objwsupof(ici_vs.a_top[-1])->o_super;

    if (ici_parse(f, a) < 0)
    {
        goto fail;
    }
    ici_file_close(f);
    a->decref();
    f->decref();
    return 0;

 fail:
    if (f != NULL)
    {
        f->decref();
    }
    if (a != NULL)
    {
        a->decref();
    }
    return -1;
}

/*
 * Parse a file as a new top-level module.  This function create new auto and
 * static scopes, and makes the current static scope the exern scope of the
 * new module.  This function takes a file name which it opens with fopen (as
 * opposed to 'ici_parse_file' which can be used to parse more generic data
 * sources).
 *
 * Return 0 if ok, else -1, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_parse_fname(const char *fname)
{
    FILE                *stream;
    int                 r;

    if ((stream = fopen(fname, "r")) == NULL)
    {
        return ici_get_last_errno("fopen", fname);
    }
    r = ici_parse_file(fname, (char *)stream, stdio_ftype);
    fclose(stream);
    return r;
}

int
ici_parse_exec()
{
    parse *p;
    array *a;

    if ((a = ici_array_new(0)) == NULL)
    {
        return 1;
    }

    p = ici_parseof(ici_xs.a_top[-1]);

    for (;;)
    {
        switch (statement(p, a, NULL, NULL, 1))
        {
        case 1:
            if (a->a_top == a->a_base)
            {
                continue;
            }
#           if DISASSEMBLE
            disassemble(4, a);
#           endif
            ici_get_pc(a, ici_xs.a_top);
            ++ici_xs.a_top;
            a->decref();
            return 0;

        case 0:
            next(p, a);
            if (p->p_module_depth > 0)
            {
                if (this != T_OFFSQUARE)
                {
                    reject(p);
                    not_followed_by("[module statements", "\"]\"");
                    goto fail;
                }
            }
            else if (this != T_EOF)
            {
                reject(p);
                ici_set_error("syntax error");
                goto fail;
            }
            --ici_xs.a_top;
            a->decref();
            return 0;

        default:
        fail:
            a->decref();
            ici_expand_error(p->p_lineno, p->p_file->f_name);
            return 1;
        }
    }
}

parse *
ici_new_parse(file *f)
{
    parse    *p;

    if ((p = (parse *)ici_talloc(parse)) == NULL)
    {
        return NULL;
    }
    memset(p, 0, sizeof(parse));
    ICI_OBJ_SET_TFNZ(p, ICI_TC_PARSE, 0, 1, 0);
    ici_rego(p);
    p->p_file = f;
    p->p_sol = 1;
    p->p_lineno = 1;
    p->p_func = NULL;
    p->p_ungot.t_what = T_NONE;
    return p;
}

size_t parse_type::mark(object *o)
{
    o->setmark();
    auto mem = typesize();
    if (ici_parseof(o)->p_func != NULL)
    {
        mem += ici_mark(ici_parseof(o)->p_func);
    }
    if (ici_parseof(o)->p_file != NULL)
    {
        mem += ici_mark(ici_parseof(o)->p_file);
    }
    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void parse_type::free(object *o)
{
    if (ici_parseof(o)->p_got.t_what & TM_HASOBJ)
    {
        ici_parseof(o)->p_got.t_obj->decref();
    }
    if (ici_parseof(o)->p_ungot.t_what & TM_HASOBJ)
    {
        ici_parseof(o)->p_ungot.t_obj->decref();
    }
    ici_parseof(o)->p_ungot.t_what = T_NONE;
    ici_tfree(o, parse);
}

const char *
ici_token_name(int t)
{
    switch (t_type(t))
    {
    case t_type(T_NONE):        return "none";
    case t_type(T_NAME):        return "name";
    case t_type(T_REGEXP):      return "regexp";
    case t_type(T_STRING):      return "string";
    case t_type(T_SEMICOLON):   return ";";
    case t_type(T_EOF):         return "eof";
    case t_type(T_INT):         return "int";
    case t_type(T_FLOAT):       return "float";
    case t_type(T_BINOP):       return binop_name(t_subtype(t));
    case t_type(T_ERROR):       return "error";
    case t_type(T_NULL):        return "NULL";
    case t_type(T_ONROUND):     return "(";
    case t_type(T_OFFROUND):    return ")";
    case t_type(T_ONCURLY):     return "{";
    case t_type(T_OFFCURLY):    return "}";
    case t_type(T_ONSQUARE):    return "[";
    case t_type(T_OFFSQUARE):   return "]";
    case t_type(T_DOT):         return ".";
    case t_type(T_PTR):         return "->";
    case t_type(T_EXCLAM):      return "!";
    case t_type(T_PLUSPLUS):    return "++";
    case t_type(T_MINUSMINUS):  return "--";
    case t_type(T_CONST):       return "const";
    case t_type(T_PRIMARYCOLON):return ":";
    case t_type(T_DOLLAR):      return "$";
    case t_type(T_COLONCARET):  return ":^";
    case t_type(T_AT):          return "@";
    case t_type(T_BINAT):       return "@";
    }
    assert(0);
    return "token";
}

static parse *
parse_file_argcheck()
{
    file          *f;

    if (typecheck("u", &f))
    {
        return NULL;
    }
    if (f->f_type != parse_ftype)
    {
        ici_argerror(0);
        return NULL;
    }
    return ici_parseof(f->f_file);
}

static int
f_parseopen(...)
{
    file		*f;
    file		*p;
    parse		*parse;

    if (typecheck("u", &f))
    {
	return 1;
    }
    if ((parse = ici_new_parse(f)) == NULL)
    {
	return 1;
    }
    if ((p = ici_file_new((char *)parse, parse_ftype, f->f_name, NULL)) == NULL)
    {
        parse->decref();
	return 1;
    }
    return ici_ret_with_decref(p);
}

static int
f_parsetoken(...)
{
    parse         *p;
    int                 t;

    if ((p = parse_file_argcheck()) == NULL)
    {
        return 1;
    }
    if ((t = next(p, NULL)) == T_ERROR)
    {
        return 1;
    }
    return t == T_EOF ? ici_null_ret() : ici_str_ret(ici_token_name(t));
}

static int
f_tokenobj(...)
{
    parse         *p;

    if ((p = parse_file_argcheck()) == NULL)
    {
        return 1;
    }
    switch (p->p_got.t_what)
    {
    case T_INT:
        return ici_int_ret(p->p_got.t_int);

    case T_FLOAT:
        return ici_float_ret(p->p_got.t_float);

    case T_REGEXP:
    case T_NAME:
    case T_STRING:
        return ici_ret_no_decref(p->p_got.t_obj);
    }
    return ici_null_ret();
}

static int
f_rejecttoken(...)
{
    parse         *p;

    if ((p = parse_file_argcheck()) == NULL)
    {
        return 1;
    }
    reject(p);
    return ici_null_ret();
}

static int
f_parsevalue(...)
{
    parse         *p;
    object           *o = NULL;

    if ((p = parse_file_argcheck()) == NULL)
    {
        return 1;
    }
    switch (const_expression(p, &o, T_COMMA))
    {
    case  0: ici_set_error("missing expression");
    case -1: return 1;
    }
    return ici_ret_with_decref(o);
}

static int
f_rejectchar(...)
{
    file          *f;
    str           *s;

    if (typecheck("uo", &f, &s))
    {
        return 1;
    }
    if (f->f_type != parse_ftype)
    {
        ici_argerror(0);
        return 1;
    }
    if (!ici_isstring(s) || s->s_nchars != 1)
    {
        return ici_argerror(1);
    }
    f->ungetch(s->s_chars[0]);
    return ici_ret_no_decref(s);
}

ICI_DEFINE_CFUNCS(parse)
{
    ICI_DEFINE_CFUNC(parseopen,    f_parseopen),
    ICI_DEFINE_CFUNC(parsetoken,   f_parsetoken),
    ICI_DEFINE_CFUNC(parsevalue,   f_parsevalue),
    ICI_DEFINE_CFUNC(tokenobj,     f_tokenobj),
    ICI_DEFINE_CFUNC(rejecttoken,  f_rejecttoken),
    ICI_DEFINE_CFUNC(rejectchar,   f_rejectchar),
    ICI_CFUNCS_END()
};

} // namespace ici

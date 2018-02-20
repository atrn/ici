#define ICI_CORE
#include "fwd.h"
#include "parse.h"
#include "func.h"
#include "cfunc.h"
#include "str.h"
#include "map.h"
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
#include "repl.h"
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
static int      compound_statement(parse *, map *);
static int      expression(parse *, expr **, int);
static int      const_expression(parse *, object **, int);
static int      statement(parse *, array *, map *, const char *, int);

#define DISASSEMBLE   0

#if defined(DISASSEMBLE) && DISASSEMBLE != 0
static char *opname(op *op)
{
    switch (op->op_ecode) {
    case OP_OTHER: return "OP_OTHER";
    case OP_CALL: return "OP_CALL";
    case OP_NAMELVALUE: return "OP_NAMELVALUE";
    case OP_DOT: return "OP_DOT";
    case OP_DOTKEEP: return "OP_DOTKEEP";
    case OP_DOTRKEEP: return "OP_DOTRKEEP";
    case OP_ASSIGN: return "OP_ASSIGN";
    case OP_ASSIGN_TO_NAME: return "OP_ASSIGN_TO_NAME";
    case OP_ASSIGNLOCAL: return "OP_ASSIGNLOCAL";
    case OP_EXEC: return "OP_EXEC";
    case OP_LOOP: return "OP_LOOP";
    case OP_REWIND: return "OP_REWIND";
    case OP_ENDCODE: return "OP_ENDCODE";
    case OP_IF: return "OP_IF";
    case OP_IFELSE: return "OP_IFELSE";
    case OP_IFNOTBREAK: return "OP_IFNOTBREAK";
    case OP_IFBREAK: return "OP_IFBREAK";
    case OP_BREAK: return "OP_BREAK";
    case OP_QUOTE: return "OP_QUOTE";
    case OP_BINOP: return "OP_BINOP";
    case OP_AT: return "OP_AT";
    case OP_SWAP: return "OP_SWAP";
    case OP_BINOP_FOR_TEMP: return "OP_BINOP_FOR_TEMP";
    case OP_AGGR_KEY_CALL: return "OP_AGGR_KEY_CALL";
    case OP_COLON: return "OP_COLON";
    case OP_COLONCARET: return "OP_COLONCARET";
    case OP_METHOD_CALL: return "OP_METHOD_CALL";
    case OP_SUPER_CALL: return "OP_SUPER_CALL";
    case OP_ASSIGNLOCALVAR: return "OP_ASSIGNLOCALVAR";
    case OP_CRITSECT: return "OP_CRITSECT";
    case OP_WAITFOR: return "OP_WAITFOR";
    case OP_POP: return "OP_POP";
    case OP_CONTINUE: return "OP_CONTINUE";
    case OP_LOOPER: return "OP_LOOPER";
    case OP_ANDAND: return "OP_ANDAND";
    case OP_SWITCH: return "OP_SWITCH";
    case OP_SWITCHER: return "OP_SWITCHER";
    default: return "op by function";
    }
}

static void
disassemble(int indent, array *a) {
    object      **e;
    char        n1[objnamez];
    int         i;

    for (i = 0, e = a->a_bot; e < a->a_top; ++i, ++e) {
        printf("%*d: ", indent, i);
        if (issrc(*e)) {
            printf("%s, %d\n", srcof(*e)->s_filename->s_chars, srcof(*e)->s_lineno);
        } else if (isop(*e)) {
            printf("%s %d\n", opname(opof(*e)), opof(*e)->op_code);
        } else {
            printf("%s\n", objname(n1, *e));
        }
        if (isarray(*e)) {
            disassemble(indent + 4, arrayof(*e));
        }
    }
}
#endif // DISASSEMBLE

/*
 * In general, parseing functions return -1 on error (and set the global
 * error string), 0 if they encountered an early head symbol conflict (and
 * the parse stream has not been disturbed), and 1 if they actually got
 * what they were looking for.
 */

/*
 * 'curtok' is a convenience to get the current token. This macro used
 * to called 'this' in the C version of this code. While that works in
 * C++ as well, since it was a macro, calling something 'this' in C++
 * is both confusing and asking for trouble.
 *
 * Routines in this file conventionally use the variable name 'p' for
 * the pointer the current parse structure. Given that, 'curtok' is
 * the last token fetched. That is, the current head symbol.
 */
#define curtok p->p_got.t_what

/*
 * next(p, a) and reject(p) are the basic token fetching (and rejecting)
 * functions (or macros). See lex() for the meanins of the 'a'. 'p' is a
 * pointer to the subject parse sructure.
 */
inline int next(parse *p, array *a) {
    if (p->p_ungot.t_what != T_NONE) {
        p->p_got = p->p_ungot;
        p->p_ungot.t_what = T_NONE;
        return curtok;
    }
    return lex(p, a);
}

inline void reject(parse *p) {
    p->p_ungot = p->p_got;
    curtok = T_NONE;
}

static int not_followed_by(const char *a, const char *b) {
    set_error("\"%s\" %s %s", a, not_by, b);
    return -1;
}

static int not_allowed(const char *what) {
    set_error("%s outside of %sable statement", what, what);
    return -1;
}

inline void increment_break_depth(parse *p) {
    ++p->p_break_depth;
}

inline void decrement_break_depth(parse *p) {
    --p->p_break_depth;
}

inline void increment_break_continue_depth(parse *p) {
    ++p->p_break_depth;
    ++p->p_continue_depth;
}

inline void decrement_break_continue_depth(parse *p) {
    --p->p_break_depth;
    --p->p_continue_depth;
}

/*
 * Returns a non-decref atomic array of identifiers parsed from a comma
 * seperated list, or nullptr on error.  The array may be empty.
 */
static array *ident_list(parse *p) {
    array *a;

    if ((a = new_array()) == nullptr) {
        return nullptr;
    }
    for (;;) {
        if (next(p, nullptr) != T_NAME) {
            reject(p);
            return a;
        }
        if (a->push_check()) {
            goto fail;
        }
        a->push(p->p_got.t_obj, with_decref);
        curtok = T_NONE; /* Take ownership of name. */
        if (next(p, nullptr) != T_COMMA) {
            reject(p);
            return arrayof(atom(a, 1));
        }
    }

fail:
    decref(a);
    return nullptr;
}

/*
 * The parse stream may be positioned just before the on-round of the
 * argument list of a function. Try to parse the function.
 *
 * Return -1, 0 or 1, usual conventions.  On success, returns a parsed
 * non-decref function in parse.p_got.t_obj.
 */
static int function(parse *p, str *name) {
    array *a;
    func  *f;
    func  *saved_func;
    object **fp;

    a = nullptr;
    f = nullptr;
    if (next(p, nullptr) != T_ONROUND) {
        reject(p);
        return 0;
    }
    if ((a = ident_list(p)) == nullptr) {
        return -1;
    }
    saved_func = p->p_func;
    if (next(p, nullptr) != T_OFFROUND) {
        reject(p);
        not_followed_by("ident ( [args]", "\")\"");
        goto fail;
    }
    if ((f = new_func()) == nullptr) {
        goto fail;
    }
    if ((f->f_autos = new_map()) == nullptr) {
        goto fail;
    }
    decref(f->f_autos);
    if (ici_assign(f->f_autos, SS(_func_), f)) {
        goto fail;
    }
    for (fp = a->a_base; fp < a->a_top; ++fp) {
        if (ici_assign(f->f_autos, *fp, null))
            goto fail;
    }
    ici_assign(f->f_autos, SS(vargs), null);
    f->f_autos->o_super = objwsupof(vs.a_top[-1])->o_super;
    p->p_func = f;
    f->f_args = a;
    decref(a);
    a = nullptr;
    f->f_name = name;
    switch (compound_statement(p, nullptr)) {
    case 0: not_followed_by("ident ( [args] )", "\"{\"");
    case -1: goto fail;
    }
    f->f_code = arrayof(p->p_got.t_obj);
    decref(f->f_code);
    if (f->f_code->a_top[-1] == &o_end) {
        --f->f_code->a_top;
    }
    if (f->f_code->push_check(3)) {
        goto fail;
    }
    f->f_code->push(null);
    f->f_code->push(&o_return);
    f->f_code->push(&o_end);
#   if DISASSEMBLE
    printf("%s()\n", name == nullptr ? "?" : name->s_chars);
    disassemble(4, f->f_code);
#   endif
    f->f_autos = mapof(atom(f->f_autos, 2));
    p->p_got.t_obj = atom(f, 1);
    p->p_func = saved_func;
    return 1;

fail:
    if (a != nullptr) {
        decref(a);
    }
    if (f != nullptr) {
        decref(f);
    }
    p->p_func = saved_func;
    return -1;
}

/*
 * ows is the struct (or whatever) the idents are going into.
 */
static int data_def(parse *p, objwsup *ows) {
    object   *o;     /* The value it is initialised with. */
    object   *n;     /* The name. */
    int      wasfunc;
    int      hasinit;

    n = nullptr;
    o = nullptr;
    wasfunc = 0;
    /*
     * Work through the list of identifiers being declared.
     */
    for (;;) {
        if (next(p, nullptr) != T_NAME) {
            reject(p);
            set_error("syntax error in variable definition");
            goto fail;
        }
        n = p->p_got.t_obj;
        curtok = T_NONE; /* Take ownership of name. */
        /*
         * Gather any initialisation or function.
         */
        hasinit = 0;
        switch (next(p, nullptr)) {
        case T_EQ:
            switch (const_expression(p, &o, T_COMMA)) {
            case 0: not_followed_by("ident =", an_expression);
            case -1: goto fail;
            }
            hasinit = 1;
            break;

        case T_ONROUND:
            reject(p);
            if (function(p, stringof(n)) < 0) {
                goto fail;
            }
            o = p->p_got.t_obj;
            wasfunc = 1;
            hasinit = 1;
            break;

        default:
            o = null;
            incref(o);
            reject(p);
        }

        /*
         * Assign to the new variable if it doesn't appear to exist
         * or has an explicit initialisation.
         */
        if (hasinit || ici_fetch_base(ows, n) == null) {
            if (ici_assign_base(ows, n, o)) {
                goto fail;
            }
        }
        decref(n);
        n = nullptr;
        decref(o);
        o = nullptr;

        if (wasfunc) {
            return 1;
        }

        switch (next(p, nullptr)) {
        case T_COMMA: continue;
        case T_SEMICOLON: return 1;
        }
        reject(p);
        set_error("variable definition not followed by \";\" or \",\"");
        goto fail;
    }

 fail:
    if (n != nullptr) {
        decref(n);
    }
    if (o != nullptr) {
        decref(o);
    }
    return -1;
}

static int compound_statement(parse *p, map *sw)
{
    array *a;

    a = nullptr;
    if (next(p, nullptr) != T_ONCURLY) {
        reject(p);
        return 0;
    }
    if ((a = new_array()) == nullptr) {
        goto fail;
    }
    for (;;) {
        switch (statement(p, a, sw, nullptr, 0)) {
        case -1: goto fail;
        case 1: continue;
        }
        break;
    }
    if (next(p, a) != T_OFFCURLY) {
        reject(p);
        set_error("badly formed statement");
        goto fail;
    }
    /*
     *  Drop any trailing source marker.
     */
    if (a->a_top > a->a_bot && issrc(a->a_top[-1])) {
        --a->a_top;
    }

    if (a->push_check()) {
        goto fail;
    }
    a->push(&o_end);
    p->p_got.t_obj = a;
    return 1;

 fail:
    if (a != nullptr) {
        decref(a);
    }
    return -1;
}

/*
 * Free an exprssesion tree and decref all the objects that it references.
 */
static void free_expr(expr *e) {
    expr *e1;

    while (e != nullptr) {
        if (e->e_arg[1] != nullptr) {
            free_expr(e->e_arg[1]);
        }
        if (e->e_obj != nullptr) {
            decref(e->e_obj);
        }
        e1 = e;
        e = e->e_arg[0];
        ici_tfree(e1, expr);
    }
}

/*
 * We have just got and accepted the '('. Now get the rest up to and
 * including the ')'.
 */
static int bracketed_expr(parse *p, expr **ep) {
    switch (expression(p, ep, T_NONE)) {
    case 0: not_followed_by("(", an_expression);
    case -1: return -1;
    }
    if (next(p, nullptr) != T_OFFROUND) {
        reject(p);
        return not_followed_by("( expr", "\")\"");
    }
    return 1;
}

/*
 * Parse a primaryexpression in the parse context 'p' and store the expression
 * tree of 'expr' type nodes under the pointer indicated by 'ep'. Usual
 * parseing return conventions (see comment near start of file). See the
 * comment on expr() for the meaning of exclude.
 */
static int primary(parse *p, expr **ep, int exclude) {
    expr       *e;
    array      *a;
    map        *d;
    set        *s;
    object     *n;
    object     *o;
    const char *token_name = 0;
    int        wasfunc;
    object     *name;
    int        token;

    *ep = nullptr;
    if ((e = ici_talloc(expr)) == nullptr) {
        return -1;
    }
    e->e_arg[0] = nullptr;
    e->e_arg[1] = nullptr;
    e->e_obj = nullptr;
    switch (next(p, nullptr)) {
    case T_INT:
        e->e_what = T_INT;
        if ((e->e_obj = new_int(p->p_got.t_int)) == nullptr) {
            goto fail;
        }
        break;

    case T_FLOAT:
        e->e_what = T_FLOAT;
        if ((e->e_obj = new_float(p->p_got.t_float)) == nullptr) {
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
        curtok = T_NONE; /* Take ownership of obj. */
        o = p->p_got.t_obj;
        while (next(p, nullptr) == token || (token == T_REGEXP && curtok == T_STRING)) {
            int        i;

            i = stringof(p->p_got.t_obj)->s_nchars;
            if (chkbuf(stringof(o)->s_nchars + i + 1)) {
                goto fail;
            }
            memcpy(buf, stringof(o)->s_chars,   stringof(o)->s_nchars);
            memcpy(buf + stringof(o)->s_nchars, stringof(p->p_got.t_obj)->s_chars, i);
            i += stringof(o)->s_nchars;
            decref(o);
            curtok = T_NONE; /* Take ownership of obj. */
            decref(p->p_got.t_obj);
            if ((o = new_str(buf, i)) == nullptr) {
                goto fail;
            }
            curtok = T_NONE;
        }
        reject(p);
        if (token == T_REGEXP) {
            e->e_obj = new_regexp(stringof(o), 0);
            decref(o);
            if (e->e_obj == nullptr) {
                goto fail;
            }
        }
        else {
            e->e_obj = o;
        }
        break;

    case T_NAME:
        curtok = T_NONE; /* Take ownership of name. */
        if (p->p_got.t_obj == SS(_NULL_)) {
            e->e_what = T_NULL;
            decref(p->p_got.t_obj);
            break;
        }
        if (p->p_got.t_obj == SS(_false_)) {
            e->e_what = T_INT;
            decref(p->p_got.t_obj);
            e->e_obj = o_zero;
            incref(e->e_obj);
            break;
        }
        if (p->p_got.t_obj == SS(_true_)) {
            e->e_what = T_INT;
            decref(p->p_got.t_obj);
            e->e_obj = o_one;
            incref(e->e_obj);
            break;
        }
        e->e_what = T_NAME;
        e->e_obj = p->p_got.t_obj;
        break;

    case T_ONROUND:
        ici_tfree(e, expr);
        e = nullptr;
        if (bracketed_expr(p, &e) < 1) {
            goto fail;
        }
        break;

    case T_ONSQUARE:
        if (next(p, nullptr) != T_NAME) {
            reject(p);
            not_followed_by("[", "an identifier");
            goto fail;
        }
        curtok = T_NONE; /* Take ownership of name. */
        if (p->p_got.t_obj == SS(array)) {
            decref(p->p_got.t_obj);
            if ((a = new_array()) == nullptr) {
                goto fail;
            }
            for (;;) {
                switch (const_expression(p, &o, T_COMMA)) {
                case -1:
                    decref(a);
                    goto fail;

                case 1:
                    if (a->push_check()) {
                        decref(a);
                        goto fail;
                    }
                    a->push(o, with_decref);
                    if (next(p, nullptr) == T_COMMA) {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, nullptr) != T_OFFSQUARE) {
                reject(p);
                decref(a);
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
            name == SS(map)
            ||
            name == SS(module)
        )
        {
            map    *super;

            decref(p->p_got.t_obj);
            d = nullptr;
            super = nullptr;
            if (next(p, nullptr) == T_COLON || curtok == T_EQ) {
                int     is_eq;
                char    n[objnamez];

                is_eq = curtok == T_EQ;
                switch (const_expression(p, &o, T_COMMA)) {
                case 0:
                    sprintf(n, "[%s %c", stringof(name)->s_chars, is_eq ? '=' : ':');
                    not_followed_by(n, an_expression);
                case -1:
                    goto fail;
                }
                if (!hassuper(o)) {
                    set_error("attempt to do [%s %c %s",
                                  stringof(name)->s_chars,
                                  is_eq ? '=' : ':',
                                  objname(n, o));
                    decref(o);
                    goto fail;
                }
                if (is_eq) {
                    d = mapof(o);
                } else {
                    super = mapof(o);
                }
                switch (next(p, nullptr)) {
                case T_OFFSQUARE:
                    reject(p);
                case T_COMMA:
                    break;

                default:
                    reject(p);
                    if (super != nullptr) {
                        decref(super);
                    }
                    if (d != nullptr) {
                        decref(o);
                    }
                    sprintf(n, "[%s %c expr", stringof(name)->s_chars, is_eq ? '=' : ':');
                    not_followed_by(n, "\",\" or \"]\"");
                    goto fail;
                }
            }
            else {
                reject(p);
            }
            if (d == nullptr) {
                if ((d = new_map()) == nullptr) {
                    goto fail;
                }
                if (super == nullptr) {
                    if (name != SS(map)) {
                        d->o_super = objwsupof(vs.a_top[-1])->o_super;
                    }
                } else {
                    d->o_super = objwsupof(super);
                    decref(super);
                }
            }

            if (name == SS(module)) {
                map    *autos;

                if ((autos = new_map()) == nullptr) {
                    decref(d);
                    goto fail;
                }
                autos->o_super = objwsupof(d);
                vs.push(autos, with_decref);
                ++p->p_module_depth;
                o = evaluate(p, 0);
                --p->p_module_depth;
                --vs.a_top;
                if (o == nullptr) {
                    decref(d);
                    goto fail;
                }
                decref(o);
                e->e_what = T_CONST;
                e->e_obj = d;
                break;
            }
            if (name == SS(class)) {
                map    *autos;

                /*
                 * A class definition operates within the scope context of
                 * the class. Create autos with the new struct as the super.
                 */
                if ((autos = new_map()) == nullptr) {
                    decref(d);
                    goto fail;
                }
                autos->o_super = objwsupof(d);
                if (vs.push_check(80)) { /* ### Formalise */
                    decref(d);
                    goto fail;
                }
                vs.push(autos, with_decref);
            }
            for (;;) {
                switch (next(p, nullptr)) {
                case T_OFFSQUARE:
                    break;

                case T_ONROUND:
                    switch (const_expression(p, &o, T_NONE)) {
                    case 0: not_followed_by("[map ... (", an_expression);
                    case -1: decref(d); goto fail;
                    }
                    if (next(p, nullptr) != T_OFFROUND) {
                        reject(p);
                        not_followed_by("[map ... (expr", "\")\"");
                        decref(d);
                        goto fail;
                    }
                    n = o;
                    goto gotkey;

                case T_NAME:
                    n = p->p_got.t_obj;
                    curtok = T_NONE; /* Take ownership of name. */
                gotkey:
                    wasfunc = 0;
                    if (next(p, nullptr) == T_ONROUND) {
                        reject(p);
                        if (function(p, stringof(n)) < 0) {
                            decref(d);
                            goto fail;
                        }
                        o = p->p_got.t_obj;
                        wasfunc = 1;
                    }
                    else if (curtok == T_EQ) {
                        switch (const_expression(p, &o, T_COMMA)) {
                        case 0: not_followed_by("[map ... ident =", an_expression);
                        case -1: decref(d); goto fail;
                        }
                    }
                    else if (curtok == T_COMMA || curtok == T_OFFSQUARE) {
                        reject(p);
                        o = null;
                        incref(o);
                    } else {
                        reject(p);
                        not_followed_by("[map ... key", "\"=\", \"(\", \",\" or \"]\"");
                        decref(d);
                        decref(n);
                        goto fail;
                    }
                    if (ici_assign_base(d, n, o)) {
                        goto fail;
                    }
                    decref(n);
                    decref(o);
                    switch (next(p, nullptr)) {
                    case T_OFFSQUARE:
                        reject(p);
                    case T_COMMA:
                        continue;

                    default:
                        if (wasfunc) {
                            reject(p);
                            continue;
                        }
                    }
                    reject(p);
                    not_followed_by("[map ... key = expr", "\",\" or \"]\"");
                    decref(d);
                    goto fail;

                default:
                    reject(p);
                    not_followed_by("[map ...", "an initialiser");
                    decref(d);
                    goto fail;
                }
                break;
            }
            if (name == SS(class)) {
                /*
                 * That was a class definition. Restore the scope context.
                 */
                --vs.a_top;
            }
            e->e_what = T_CONST;
            e->e_obj = d;
        }
        else if (p->p_got.t_obj == SS(set)) {
            decref(p->p_got.t_obj);
            if ((s = new_set()) == nullptr) {
                goto fail;
            }
            for (;;) {
                switch (const_expression(p, &o, T_COMMA)) {
                case -1:
                    decref(s);
                    goto fail;

                case 1:
                    if (ici_assign(s, o, o_one)) {
                        decref(s);
                        goto fail;
                    }
                    decref(o);
                    if (next(p, nullptr) == T_COMMA) {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, nullptr) != T_OFFSQUARE) {
                reject(p);
                decref(s);
                not_followed_by("[set expr, expr ...", "\"]\"");
                goto fail;
            }
            e->e_what = T_CONST;
            e->e_obj = s;
        }
        else if (p->p_got.t_obj == SS(func)) {
            decref(p->p_got.t_obj);
            switch (function(p, SS(empty_string))) {
            case 0: not_followed_by("[func", "function body");
            case -1:
                goto fail;
            }
            e->e_what = T_CONST;
            e->e_obj = p->p_got.t_obj;
            if (next(p, nullptr) != T_OFFSQUARE) {
                reject(p);
                not_followed_by("[func function-body ", "\"]\"");
                goto fail;
            }
        } else {
            str    *s;     /* Name after the on-square . */
            file   *f;     /* The parse file. */
            object *c;     /* The callable parser function. */

            f = nullptr;
            n = nullptr;
            c = nullptr;
            s = stringof(p->p_got.t_obj);
            if ((o = eval(s)) == nullptr) {
                goto fail_user_parse;
            }
            if (o->can_call()) {
                c = o;
                o = nullptr;
            } else {
                if ((c = ici_fetch(o, SS(parser))) == nullptr) {
                    goto fail_user_parse;
                }
            }
            f = new_file(p, parse_ftype, p->p_file->f_name, p);
            if (f == nullptr) {
                goto fail_user_parse;
            }
            incref(c);
            if (call(c, "o=o", &n, f)) {
                goto fail_user_parse;
            }
            e->e_what = T_CONST;
            e->e_obj = n;
            decref(s);
            if (o != nullptr) {
                decref(o);
            }
            decref(f);
            decref(c);
            if (next(p, nullptr) != T_OFFSQUARE) {
                reject(p);
                not_followed_by("[name ... ", "\"]\"");
                goto fail;
            }
            break;

        fail_user_parse:
            decref(s);
            if (o != nullptr) {
                decref(o);
            }
            if (f != nullptr) {
                decref(f);
            }
            if (c != nullptr) {
                decref(c);
            }
            goto fail;
        }
        break;

    default:
        reject(p);
        ici_tfree(e, expr);
        return 0;
    }
    *ep = e;
    e = nullptr;
    for (;;) {
        int     oldcurtok;

        switch (next(p, nullptr)) {
        case T_ONSQUARE:
            if ((e = ici_talloc(expr)) == nullptr) {
                goto fail;
            }
            e->e_what = T_ONSQUARE;
            e->e_arg[0] = *ep;
            e->e_arg[1] = nullptr;
            e->e_obj = nullptr;
            *ep = e;
            e = nullptr;
            switch (expression(p, &(*ep)->e_arg[1], T_NONE)) {
            case 0: not_followed_by("[", an_expression);
            case -1: goto fail;
            }
            if (next(p, nullptr) != T_OFFSQUARE) {
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
            if (curtok == exclude) {
                reject(p);
                return 1;
            }
            if ((e = ici_talloc(expr)) == nullptr) {
                goto fail;
            }
            if ((oldcurtok = curtok) == T_AT) {
                e->e_what = T_BINAT;
            }
            else if (oldcurtok == T_COLON) {
                e->e_what = T_PRIMARYCOLON;
            } else {
                e->e_what = curtok;
            }
            e->e_arg[0] = *ep;
            e->e_arg[1] = nullptr;
            e->e_obj = nullptr;
            *ep = e;
            e = nullptr;
            switch (next(p, nullptr)) {
            case T_NAME:
                curtok = T_NONE; /* Take ownership of name. */
                if ((e = ici_talloc(expr)) == nullptr) {
                    goto fail;
                }
                e->e_what = T_STRING;
                e->e_arg[0] = nullptr;
                e->e_arg[1] = nullptr;
                e->e_obj = nullptr;
                e->e_obj = p->p_got.t_obj;
                (*ep)->e_arg[1] = e;
                e = nullptr;
                break;

            case T_ONROUND:
                if (bracketed_expr(p, &(*ep)->e_arg[1]) < 1) {
                    goto fail;
                }
                break;

            default:
                reject(p);
                switch (oldcurtok) {
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
            if ((e = ici_talloc(expr)) == nullptr) {
                goto fail;
            }
            e->e_what = T_ONROUND;
            e->e_arg[0] = *ep;
            e->e_arg[1] = nullptr;
            e->e_obj = nullptr;
            *ep = e;
            e = nullptr;
            for (;;) {
                expr  *e1;

                e1 = nullptr;
                switch (expression(p, &e1, T_COMMA)) {
                case -1:
                    goto fail;

                case 1:
                    if ((e = ici_talloc(expr)) == nullptr) {
                        goto fail;
                    }
                    e->e_arg[1] = (*ep)->e_arg[1];
                    (*ep)->e_arg[1] = e;
                    e->e_what = T_COMMA;
                    e->e_arg[0] = e1;
                    e->e_obj = nullptr;
                    e = nullptr;
                    if (next(p, nullptr) == T_COMMA) {
                        continue;
                    }
                    reject(p);
                    break;
                }
                break;
            }
            if (next(p, nullptr) != T_OFFROUND) {
                reject(p);
                set_error("error in function call arguments");
                goto fail;
            }
            if (next(p, nullptr) == T_ONCURLY) {
                /*
                 * Gratuitous check to get a better error message.
                 */
                set_error("function definition without a storage class");
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
    if (e != nullptr) {
        if (e->e_obj != nullptr) {
            decref(e->e_obj);
        }
        ici_tfree(e, expr);
    }
    free_expr(*ep);
    *ep = nullptr;
    return -1;
}

/*
 * Parse a sub-expression consisting or a sequence of unary operators round
 * a primary (a factor) in the parse context 'p' and store the expression
 * tree of 'expr' type nodes under the pointer indicated by 'ep'. Usual
 * parseing return conventions (see comment near start of file). See the
 * comment on expr() for the meaning of exclude.
 */
static int unary(parse *p, expr **ep, int exclude) {
    expr      *e;
    int         what;

    switch (next(p, nullptr)) {
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
        what = curtok;
        switch (unary(p, ep, exclude)) {
        case 0: set_error("badly formed expression");
        case -1: return -1;
        }
        if ((e = ici_talloc(expr)) == nullptr) {
            return -1;
        }
        e->e_what = what;
        e->e_arg[0] = *ep;
        e->e_arg[1] = nullptr;
        e->e_obj = nullptr;
        *ep = e;
        break;

    default:
        reject(p);
        switch (primary(p, ep, exclude)) {
        case 0: return 0;
        case -1: return -1;
        }
    }
    switch (next(p, nullptr))
    {
    case T_PLUSPLUS:
    case T_MINUSMINUS:
        if ((e = ici_talloc(expr)) == nullptr) {
            return -1;
        }
        e->e_what = curtok;
        e->e_arg[0] = nullptr;
        e->e_arg[1] = *ep;
        e->e_obj = nullptr;
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
 * tree of 'expr' type nodes under the pointer indicated by 'ep'. Usual
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
static int expression(parse *p, expr **ep, int exclude) {
    expr      *e;
    expr      **ebase;
    expr      *elimit;
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
    if ((r = unary(p, ebase = ep, exclude)) <= 0) {
        return r;
    }
    elimit = *ebase;

    /*
     * While there is a following binary operator, merge it and the
     * following factor into the expression.
     */
    while (t_type(next(p, nullptr)) == T_BINOP && curtok != exclude) {
        /*
         * Cause assignments to be right associative.
         */
        if ((tp = t_prec(curtok)) == t_prec(T_EQ)) {
            --tp;
        }

        /*
         * Slide down the right hand side of the tree to find where this
         * operator binds.
         */
        in_quest_colon = curtok == T_QUESTION;
        for
        (
            ep = ebase;
            (e = *ep) != elimit && tp < t_prec(e->e_what);
            ep = &e->e_arg[1]
        )
        {
            if (e->e_what == T_QUESTION) {
                in_quest_colon = 1;
            }
        }

        /*
         * Allocate a new node and rebuild this bit with the new operator
         * and the following factor.
         */
        if ((e = ici_talloc(expr)) == nullptr) {
            return -1;
        }
        e->e_what = curtok;
        e->e_arg[0] = *ep;
        e->e_arg[1] = nullptr;
        e->e_obj = nullptr;
        switch (unary(p, &e->e_arg[1], in_quest_colon ? T_COLON : exclude)) {
        case 0:
            set_error("\"expr %s\" %s %s", binop_name(t_subtype(e->e_what)), not_by, an_expression);
        case -1:
            ici_tfree(e, expr);
            return -1;
        }
        *ep = e;
        elimit = e->e_arg[1];
    }
    reject(p);
    return 1;
}

static int expression(parse *p, array *a, int why, int exclude) {
    expr      *e;

    e = nullptr;
    switch (expression(p, &e, exclude)) {
    case 0: return 0;
    case -1: goto fail;
    }
    if (compile_expr(a, e, why)) {
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
static int const_expression(parse *p, object **po, int exclude) {
    expr      *e;
    array     *a;
    int       ret;

    a = nullptr;
    e = nullptr;
    if ((ret = expression(p, &e, exclude)) <= 0) {
        return ret;
    }
    /*
     * If the expression is a data item that obviously requires no
     * further compilation and evaluation to arrive at its value,
     * we just use the value directly.
     */
    switch (e->e_what) {
    case T_NULL:
        *po = null;
        goto simple;

    case T_INT:
    case T_FLOAT:
    case T_STRING:
    case T_CONST:
        *po = e->e_obj;
    simple:
        incref(*po);
        free_expr(e);
        return 1;
    }
    if ((a = new_array()) == nullptr) {
        goto fail;
    }
    if (compile_expr(a, e, FOR_VALUE)) {
        goto fail;
    }
    if (a->push_check()) {
        goto fail;
    }
    a->push(&o_end);
    free_expr(e);
    e = nullptr;
    if ((*po = evaluate(a, 0)) == nullptr) {
        goto fail;
    }
    decref(a);
    return 1;

 fail:
    if (a != nullptr) {
        decref(a);
    }
    free_expr(e);
    return -1;
}

static int xx_brac_expr_brac(parse *p, array *a, const char *xx) {
    if (next(p, a) != T_ONROUND) {
        reject(p);
        sprintf(buf, "\"%s\" %s a \"(\"", xx, not_by);
        goto fail;
    }
    switch (expression(p, a, FOR_VALUE, T_NONE)) {
    case 0:
        sprintf(buf, "\"%s (\" %s %s", xx, not_by, an_expression);
        goto fail;

    case -1:
        return -1;
    }
    if (next(p, a) != T_OFFROUND) {
        reject(p);
        sprintf(buf, "\"%s (expr\" %s \")\"", xx, not_by);
        goto fail;
    }
    return 1;

 fail:
    set_error("%s", buf);
    return -1;
}

/*
 * a    Code array being appended to.
 * sw   Switch structure, else nullptr.
 * m    Who needs it, else nullptr.
 * endme If non-zero, put an o_end at the end of the code array before
 *      returning.
 */
static int statement(parse *p, array *a, map *sw, const char *m, int endme) {
    array      *a1;
    array      *a2;
    expr       *e;
    map        *d;
    objwsup    *ows;
    object     *o;
    integer    *i;
    int         stepz;

    switch (next(p, a)) {
    case T_ONCURLY:
        for (;;) {
            switch (statement(p, a, nullptr, nullptr, 0)) {
            case -1: return -1;
            case 1: continue;
            }
            break;
        }
        if (next(p, a) != T_OFFCURLY) {
            reject(p);
            set_error("badly formed statement");
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
        curtok = T_NONE; /* Assume we own the name. */
        if (p->p_got.t_obj == SS(extern)) {
            decref(p->p_got.t_obj);
            if ((ows = objwsupof(vs.a_top[-1])->o_super) == nullptr || (ows = ows->o_super) == nullptr) {
                set_error("global declaration, but no global variable scope");
                return -1;
            }
            goto decl;
        }
        if (p->p_got.t_obj == SS(local)) {
            decref(p->p_got.t_obj);
            if ((ows = objwsupof(vs.a_top[-1])->o_super) == nullptr) {
                set_error("local declaration, but no local variable scope");
                return -1;
            }
            goto decl;
        }
        if (p->p_got.t_obj == SS(var)) {
            decref(p->p_got.t_obj);
            if (p->p_func == nullptr) {
                ows = objwsupof(vs.a_top[-1]);
            } else {
                ows = objwsupof(p->p_func->f_autos);
            }
        decl:
            if (data_def(p, ows) == -1) {
                return -1;
            }
            break;
        }
        if (p->p_got.t_obj == SS(case)) {
            decref(p->p_got.t_obj);
            if (sw == nullptr) {
                set_error("\"case\" not at top level of switch body");
                return -1;
            }
            switch (const_expression(p, &o, T_COLON)) {
            case 0: not_followed_by("case", an_expression);
            case -1: return -1;
            }
            stepz = a->a_top - a->a_bot;
            if (stepz > 0 && issrc(a->a_top[-1])) {
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
            if ((i = new_int((long)stepz)) == nullptr) {
                decref(o);
                return -1;
            }
            if (ici_assign(sw, o, i)) {
                decref(i);
                decref(o);
                return -1;
            }
            decref(i);
            decref(o);
            if (next(p, a) != T_COLON) {
                reject(p);
                return not_followed_by("case expr", "\":\"");
            }
            break;
        }
        if (p->p_got.t_obj == SS(default)) {
            decref(p->p_got.t_obj);
            if (sw == nullptr) {
                set_error("\"default\" not at top level of switch body");
                return -1;
            }
            if (next(p, a) != T_COLON) {
                reject(p);
                return not_followed_by("default", "\":\"");
            }
            stepz = a->a_top - a->a_bot;
            if (stepz > 0 && issrc(a->a_top[-1])) {
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
            if ((i = new_int((long)stepz)) == nullptr) {
                return -1;
            }
            if (ici_assign(sw, &o_mark, i)) {
                decref(i);
                return -1;
            }
            decref(i);
            break;
        }
        if (p->p_got.t_obj == SS(if)) {
            decref(p->p_got.t_obj);
            if (xx_brac_expr_brac(p, a, "if") != 1) {
                return -1;
            }
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            if (statement(p, a1, nullptr, "if (expr)", 1) == -1) {
                decref(a1);
                return -1;
            }
            a2 = nullptr;
            /*
             * Don't pass any code array to next() on else clause to stop
             * spurious src marker.
             */
            if (next(p, nullptr) == T_NAME && p->p_got.t_obj == SS(else)) {
                curtok = T_NONE; /* Take ownership of name. */
                decref(p->p_got.t_obj);
                if ((a2 = new_array()) == nullptr) {
                    decref(a1);
                    return -1;
                }
                if (statement(p, a2, nullptr, "if (expr) stmt else", 1) == -1) {
                    decref(a1);
                    decref(a2);
                    return -1;
                }
            } else {
                reject(p);
            }
            if (a->push_check(3)) {
                decref(a1);
                if (a2 != nullptr) {
                    decref(a2);
                }
                return -1;
            }
            if (a2 != nullptr) {
                a->push(&o_ifelse);
                a->push(a1);
                a->push(a2, with_decref);
            } else {
                a->push(&o_if);
                a->push(a1);
            }
            decref(a1);
            break;
        }
        if (p->p_got.t_obj == SS(while)) {
            decref(p->p_got.t_obj);
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            if (xx_brac_expr_brac(p, a1, "while") != 1) {
                decref(a1);
                return -1;
            }
            if (a1->push_check()) {
                decref(a1);
                return -1;
            }
            a1->push(&o_ifnotbreak);
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, nullptr, "while (expr)", 0);
                decrement_break_continue_depth(p);
                if (rc == -1) {
                    decref(a1);
                    return -1;
                }
            }
            if (a1->push_check()) {
                decref(a1);
                return -1;
            }
            a1->push(&o_rewind);
            if (a->push_check(2)) {
                decref(a1);
                return -1;
            }
            a->push(&o_loop);
            a->push(a1, with_decref);
            break;
        }
        if (p->p_got.t_obj == SS(do)) {
            decref(p->p_got.t_obj);
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, nullptr, "do", 0);
                decrement_break_continue_depth(p);
                if (rc == -1) {
                    decref(a1);
                    return -1;
                }
            }
            if (next(p, a1) != T_NAME || p->p_got.t_obj != SS(while)) {
                reject(p);
                decref(a1);
                return not_followed_by("do statement", "\"while\"");
            }
            curtok = T_NONE; /* Take ownership of name. */
            decref(p->p_got.t_obj);
            if (next(p, nullptr) != T_ONROUND) {
                reject(p);
                decref(a1);
                return not_followed_by("do statement while", "\"(\"");
            }
            switch (expression(p, a1, FOR_VALUE, T_NONE)) {
            case 0: set_error("syntax error");
            case -1: decref(a1); return -1;
            }
            if (next(p, a1) != T_OFFROUND || next(p, nullptr) != T_SEMICOLON) {
                reject(p);
                decref(a1);
                return not_followed_by("do statement while (expr", "\");\"");
            }
            if (a1->push_check(2)) {
                decref(a1);
                return -1;
            }
            a1->push(&o_ifnotbreak);
            a1->push(&o_rewind);
            if (a->push_check(2)) {
                decref(a1);
                return -1;
            }
            a->push(&o_loop);
            a->push(a1, with_decref);
            break;
        }
        if (p->p_got.t_obj == SS(forall)) {
            int         rc;

            decref(p->p_got.t_obj);
            if (next(p, a) != T_ONROUND) {
                reject(p);
                return not_followed_by("forall", "\"(\"");
            }
            if ((rc = expression(p, a, FOR_LVALUE, T_COMMA)) == -1) {
                return -1;
            }
            if (rc == 0) {
                if (a->push_check(2)) {
                    return -1;
                }
                a->push(null);
                a->push(null);
            }
            if (next(p, a) == T_COMMA) {
                if (expression(p, a, FOR_LVALUE, T_COMMA) == -1) {
                    return -1;
                }
                if (next(p, a) != T_NAME || p->p_got.t_obj != SS(in)) {
                    reject(p);
                    return not_followed_by("forall (expr, expr", "\"in\"");
                }
                curtok = T_NONE; /* Take ownership of name. */
                decref(p->p_got.t_obj);
            }
            else
            {
                if (curtok != T_NAME || p->p_got.t_obj != SS(in)) {
                    reject(p);
                    return not_followed_by("forall (expr", "\",\" or \"in\"");
                }
                curtok = T_NONE; /* Take ownership of name. */
                decref(p->p_got.t_obj);
                if (a->push_check(2)) {
                    return -1;
                }
                a->push(null);
                a->push(null);
            }
            if (expression(p, a, FOR_VALUE, T_NONE) == -1) {
                return -1;
            }
            if (next(p, a) != T_OFFROUND) {
                reject(p);
                return not_followed_by("forall (expr [, expr] in expr", "\")\"");
            }
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, nullptr, "forall (expr [, expr] in expr)", 1);
                decrement_break_continue_depth(p);
                if (rc == -1) {
                    decref(a1);
                    return -1;
                }
            }
            if (a->push_check(2)) {
                decref(a1);
                return -1;
            }
            a->push(a1, with_decref);
            auto fo = new_op(op_forall, 0, 0);
            if (!fo) {
                return -1;
            }
            a->push(fo, with_decref);
            break;

        }
        if (p->p_got.t_obj == SS(for)) {
            decref(p->p_got.t_obj);
            if (next(p, a) != T_ONROUND) {
                reject(p);
                return not_followed_by("for", "\"(\"");
            }
            if (expression(p, a, FOR_EFFECT, T_NONE) == -1) {
                return -1;
            }
            if (next(p, a) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("for (expr", "\";\"");
            }

            /*
             * Get the condition expression, but don't generate code yet.
             */
            e = nullptr;
            if (expression(p, &e, T_NONE) == -1) {
                return -1;
            }
            if (next(p, a) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("for (expr; expr", "\";\"");
            }

            /*
             * a1 is the body of the loop.  Get the step expression.
             */
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            if (expression(p, a1, FOR_EFFECT, T_NONE) == -1) {
                decref(a1);
                return -1;
            }
            stepz = a1->a_top - a1->a_base;

            if (e != nullptr) {
                /*
                 * Now compile in the test expression.
                 */
                if (compile_expr(a1, e, FOR_VALUE)) {
                    free_expr(e);
                    decref(a1);
                    return -1;
                }
                free_expr(e);
                if (a1->push_check()) {
                    decref(a1);
                    return -1;
                }
                a1->push(&o_ifnotbreak);
            }
            if (next(p, a1) != T_OFFROUND) {
                reject(p);
                decref(a1);
                return not_followed_by("for (expr; expr; expr", "\")\"");
            }
            {
                int rc;
                increment_break_continue_depth(p);
                rc = statement(p, a1, nullptr, "for (expr; expr; expr)", 0);
                decrement_break_continue_depth(p);
                if (rc == -1) {
                    decref(a1);
                    return -1;
                }
            }
            if (a1->push_check()) {
                decref(a1);
                return -1;
            }
            a1->push(&o_rewind);
            if (a->push_check(2)) {
                decref(a1);
                return -1;
            }
            a->push(a1, with_decref);
            auto fo = new_op(op_for, 0, stepz);
            if (!fo) {
                return -1;
            }
            a->push(fo, with_decref);
            break;
        }
        if (p->p_got.t_obj == SS(switch)) {
            decref(p->p_got.t_obj);
            if (xx_brac_expr_brac(p, a, "switch") != 1) {
                return -1;
            }
            if ((d = new_map()) == nullptr) {
                return -1;
            }
            {
                int rc;
                increment_break_depth(p);
                rc = compound_statement(p, d);
                decrement_break_depth(p);
                switch (rc) {
                case 0:
                    not_followed_by("switch (expr)", "a compound statement");
                case -1:
                    decref(d);
                    return -1;
                }
            }
            if (a->push_check(3)) {
                decref(d);
                decref(p->p_got.t_obj);
                return -1;
            }
            a->push(p->p_got.t_obj, with_decref);
            a->push(d, with_decref);
            a->push(&o_switch);
            break;
        }
        if (p->p_got.t_obj == SS(break)) {
            decref(p->p_got.t_obj);
            if (p->p_break_depth == 0) {
                return not_allowed("break");
            }
            if (next(p, a) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("break", "\";\"");
            }
            if (a->push_check()) {
                return -1;
            }
            a->push(&o_break);
            break;

        }
        if (p->p_got.t_obj == SS(continue)) {
            decref(p->p_got.t_obj);
            if (p->p_continue_depth == 0) {
                return not_allowed("continue");
            }
            if (next(p, a) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("continue", "\";\"");
            }
            if (a->push_check()) {
                return -1;
            }
            a->push(&o_continue);
            break;
        }
        if (p->p_got.t_obj == SS(return)) {
            decref(p->p_got.t_obj);
            switch (expression(p, a, FOR_VALUE, T_NONE)) {
            case -1: return -1;
            case 0:
                if (a->push_check()) return -1;
                a->push(null);
            }
            if (next(p, a) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("return [expr]", "\";\"");
            }
            if (a->push_check()) {
                return -1;
	    }
            a->push(&o_return);
            break;
        }
        if (p->p_got.t_obj == SS(try)) {
            decref(p->p_got.t_obj);
            if ((a1 = new_array()) == nullptr) {
                return -1;
            }
            if (statement(p, a1, nullptr, "try", 1) == -1) {
                decref(a1);
                return -1;
            }
            if (next(p, nullptr) != T_NAME || p->p_got.t_obj != SS(onerror)) {
                reject(p);
                decref(a1);
                return not_followed_by("try statement", "\"onerror\"");
            }
            curtok = T_NONE; /* Take ownership of name. */
            decref(p->p_got.t_obj);
            if ((a2 = new_array()) == nullptr) {
                return -1;
            }
            if (statement(p, a2, nullptr, "try statement onerror", 1) == -1) {
                decref(a1);
                decref(a2);
                return -1;
            }
            if (a->push_check(3)) {
                decref(a1);
                decref(a2);
                return -1;
            }
            a->push(a1, with_decref);
            a->push(a2, with_decref);
            a->push(&o_onerror);
            break;
        }
        if (p->p_got.t_obj == SS(critsect)) {
            decref(p->p_got.t_obj);
            /*
             * Start a critical section with a new code array (a1) as
             * its subject. Into this code array we place the statement.
             */
            if (a->push_check(2)) {
                return -1;
            }
            if ((a1 = new_array(1)) == nullptr) {
                return -1;
            }
            a->push(a1, with_decref);
            if (statement(p, a1, nullptr, "critsect", 1) == -1) {
                return -1;
            }
            a->push(&o_critsect);
            break;
        }
        if (p->p_got.t_obj == SS(waitfor)) {
            decref(p->p_got.t_obj);
            /*
             * Start a critical section with a new code array (a1) as
             * its subject. Into this code array we place a loop followed
             * by the statement. Thus the critical section will dominate
             * the entire waitfor statement. But the wait primitive
             * temporarily releases one level of critical section around
             * the actual wait call.
             */
            if (a->push_check(2)) {
                return -1;
            }
            if ((a1 = new_array(2)) == nullptr) {
                return -1;
            }
            a->push(a1, with_decref);
            a->push(&o_critsect);
            /*
             * Start a new code array (a2) and establish it as the body of
             * a loop.
             */
            if (a1->push_check(2)) {
                return -1;
            }
            if ((a2 = new_array(2)) == nullptr) {
                return -1;
            }
            a1->push(&o_loop);
            a1->push(a2, with_decref);
            /*
             * Into the new code array (a1, the body of the loop) we build:
             *     condition expression (for value)
             *     if-break operator
             *     wait object expression (for value)
             *     waitfor operator
             */
            if (next(p, a2) != T_ONROUND) {
                reject(p);
                return not_followed_by("waitfor", "\"(\"");
            }
            switch (expression(p, a2, FOR_VALUE, T_NONE)) {
            case 0: not_followed_by("waitfor (", an_expression);
            case -1: return -1;
            }
            if (a2->push_check()) {
                return -1;
            }
            a2->push(&o_ifbreak);
            if (next(p, a2) != T_SEMICOLON) {
                reject(p);
                return not_followed_by("waitfor (expr", "\";\"");
            }
            switch (expression(p, a2, FOR_VALUE, T_NONE)) {
            case 0: not_followed_by("waitfor (expr;", an_expression);
            case -1: return -1;
            }
            if (a2->push_check(2)) {
                return -1;
            }
            a2->push(&o_waitfor);
            a2->push(&o_rewind);
            /*
             * After we break out of the loop, we execute the statement,
             * but it is still on top of the critical section. After the
             * statement is executed, the execution engine will pop off
             * the critical section catcher.
             */
            if (next(p, a2) != T_OFFROUND) {
                reject(p);
                return not_followed_by("waitfor (expr; expr", "\")\"");
            }
            if (statement(p, a1, nullptr, "waitfor (expr; expr)", 1) == -1) {
                return -1;
            }
            break;
        }
        curtok = T_NAME; /* Woops, we wan't that name afterall. */
    default:
        reject(p);
        switch (expression(p, a, FOR_EFFECT, T_NONE)) {
        case 0: goto none;
        case -1: return -1;
        }
        switch (next(p, a)) {
        case T_OFFCURLY:
        case T_EOF:
            reject(p);
            break;

        case T_SEMICOLON:
            break;

        default:
            reject(p);
            set_error("badly formed expression, or missing \";\"");
            return -1;
        }
    }
    if (endme) {
        /*
         *  Drop any trailing source marker.
         */
        if (a->a_top > a->a_bot && issrc(a->a_top[-1])) {
            --a->a_top;
        }
        if (a->push_check()) {
            return -1;
        }
        a->push(&o_end);
    }
    return 1;

 none:
    if (m != nullptr) {
        set_error("\"%s\" %s a reasonable statement", m, not_by);
        return -1;
    }
    return 0;
}

/*
 * Parse the given file 'f' in the given scope 's'. It is common to call
 * this function with 's' being 'vs.a_top[-1]', that is, the current
 * scope.
 *
 * Returns non-zero on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int parse_file(file *f, objwsup *s) {
    struct parse  *p;
    object *o;

    if ((p = new_parse(f)) == nullptr) {
        return -1;
    }
    vs.push(s);
    if ((o = evaluate(p, 0)) == nullptr) {
        --vs.a_top;
        decref(p);
        return -1;
    }
    --vs.a_top;
    decref(o);
    decref(p);
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
int parse_file(const char *mname, char *fileo, ftype *ftype) {
    objwsup       *s;     /* Statics. */
    objwsup       *a;     /* Autos. */
    file          *f;

    a = nullptr;
    f = nullptr;
    if ((f = new_file(fileo, ftype, str_get_nul_term(mname), nullptr)) == nullptr) {
        goto fail;
    }

    if ((a = objwsupof(new_map())) == nullptr) {
        goto fail;
    }
    if (ici_assign(a, SS(_file_), f->f_name)) {
        goto fail;
    }
    if ((a->o_super = s = objwsupof(new_map())) == nullptr) {
        goto fail;
    }
    decref(s);
    s->o_super = objwsupof(vs.a_top[-1])->o_super;

    if (parse_file(f, a) < 0) {
        goto fail;
    }
    close_file(f);
    decref(a);
    decref(f);
    return 0;

 fail:
    if (f != nullptr) {
        decref(f);
    }
    if (a != nullptr) {
        decref(a);
    }
    return -1;
}

/*
 * Parse a file as a new top-level module.  This function create new auto and
 * static scopes, and makes the current static scope the exern scope of the
 * new module.  This function takes a file name which it opens with fopen (as
 * opposed to 'parse_file' which can be used to parse more generic data
 * sources).
 *
 * Return 0 if ok, else -1, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int parse_file(const char *fname) {
    FILE                *stream;
    int                 r;

    if ((stream = fopen(fname, "r")) == nullptr) {
        return get_last_errno("open", fname);
    }
    r = parse_file(fname, (char *)stream, stdio_ftype);
    fclose(stream);
    return r;
}

int parse_exec() {
    parse *p;
    array *a;

    if ((a = new_array()) == nullptr) {
        return 1;
    }

    p = parseof(xs.a_top[-1]);

    for (;;) {
        // The REPL needs to know when to issue a prompt so if
        // we are parsing on behalf of the REPL tell it we're
        // starting to parse a new statement.
        if (p->p_file->f_type == repl_ftype) {
            repl_file_new_statement(p->p_file->f_file);
        }
        switch (statement(p, a, nullptr, nullptr, 1)) {
        case 1:
            if (a->a_top == a->a_base) {
                continue;
            }
#           if DISASSEMBLE
            disassemble(4, a);
#           endif
            set_pc(a, xs.a_top);
            ++xs.a_top;
            decref(a);
            return 0;

        case 0:
            next(p, a);
            if (p->p_module_depth > 0) {
                if (curtok != T_OFFSQUARE) {
                    reject(p);
                    not_followed_by("[module statements", "\"]\"");
                    goto fail;
                }
            }
            else if (curtok != T_EOF) {
                reject(p);
                set_error("syntax error");
                goto fail;
            }
            --xs.a_top;
            decref(a);
            return 0;

        default:
        fail:
            decref(a);
            expand_error(p->p_lineno, p->p_file->f_name);
            return 1;
        }
    }
}

parse *new_parse(file *f) {
    parse    *p;

    if ((p = (parse *)ici_talloc(parse)) == nullptr) {
        return nullptr;
    }
    memset(p, 0, sizeof (parse));
    set_tfnz(p, TC_PARSE, 0, 1, 0);
    rego(p);
    p->p_file = f;
    p->p_sol = 1;
    p->p_lineno = 1;
    p->p_func = nullptr;
    p->p_ungot.t_what = T_NONE;
    return p;
}

size_t parse_type::mark(object *o) {
    auto p = parseof(o);
    return type::mark(p) + mark_optional(p->p_func) + mark_optional(p->p_file);
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void parse_type::free(object *o) {
    if (parseof(o)->p_got.t_what & TM_HASOBJ) {
        decref(parseof(o)->p_got.t_obj);
    }
    if (parseof(o)->p_ungot.t_what & TM_HASOBJ) {
        decref(parseof(o)->p_ungot.t_obj);
    }
    parseof(o)->p_ungot.t_what = T_NONE;
    ici_tfree(o, parse);
}

const char *ici_token_name(int t) {
    switch (t_type(t)) {
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
    case t_type(T_NULL):        return "nullptr";
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

static parse * parse_file_argcheck() {
    file *f;

    if (typecheck("u", &f)) {
        return nullptr;
    }
    if (f->f_type != parse_ftype) {
        argerror(0);
        return nullptr;
    }
    return parseof(f->f_file);
}

static int f_parseopen() {
    file  *f;
    file  *pf;
    parse *p;

    if (typecheck("u", &f)) {
	return 1;
    }
    if ((p = new_parse(f)) == nullptr) {
	return 1;
    }
    if ((pf = new_file((char *)p, parse_ftype, f->f_name, nullptr)) == nullptr) {
        decref(p);
	return 1;
    }
    decref(p);
    return ret_with_decref(pf);
}

static int f_parsetoken() {
    parse *p;
    int    t;

    if ((p = parse_file_argcheck()) == nullptr) {
        return 1;
    }
    if ((t = next(p, nullptr)) == T_ERROR) {
        return 1;
    }
    return t == T_EOF ? null_ret() : str_ret(ici_token_name(t));
}

static int f_tokenobj() {
    parse *p;

    if ((p = parse_file_argcheck()) == nullptr) {
        return 1;
    }
    switch (p->p_got.t_what) {
    case T_INT:
        return int_ret(p->p_got.t_int);

    case T_FLOAT:
        return float_ret(p->p_got.t_float);

    case T_REGEXP:
    case T_NAME:
    case T_STRING:
        return ret_no_decref(p->p_got.t_obj);
    }
    return null_ret();
}

static int f_rejecttoken() {
    parse *p;

    if ((p = parse_file_argcheck()) == nullptr) {
        return 1;
    }
    reject(p);
    return null_ret();
}

static int f_parsevalue() {
    parse         *p;
    object           *o = nullptr;

    if ((p = parse_file_argcheck()) == nullptr) {
        return 1;
    }
    switch (const_expression(p, &o, T_COMMA)) {
    case  0: set_error("missing expression");
    case -1: return 1;
    }
    return ret_with_decref(o);
}

static int f_rejectchar() {
    file *f;
    str  *s;

    if (typecheck("uo", &f, &s)) {
        return 1;
    }
    if (f->f_type != parse_ftype) {
        argerror(0);
        return 1;
    }
    if (!isstring(s) || s->s_nchars != 1) {
        return argerror(1);
    }
    f->ungetch(s->s_chars[0]);
    return ret_no_decref(s);
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

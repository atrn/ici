#define ICI_CORE
#include "array.h"
#include "fwd.h"
#include "int.h"
#include "null.h"
#include "op.h"
#include "parse.h"
#include "str.h"
#include "userop.h"

namespace ici
{

/*
 * A cache of binary opperators created by new_binop.
 */
static object *binops[BINOP_MAX + 1];
static object *binops_temps[BINOP_MAX + 1];

/*
 * Return a new op object corresponding to the binary operation of the
 * given token, which is a binary operator. The returned object does
 * not have an extra reference (unlike most new_* functions).
 */
static object *new_binop(int op, int why)
{
    object *o;

    op = t_subtype(op);
    if (why != FOR_TEMP)
    {
        if ((o = binops[op]) != nullptr)
        {
            return o;
        }
    }
    else
    {
        if ((o = binops_temps[op]) != nullptr)
        {
            return o;
        }
    }
#ifndef BINOPFUNC
    /*
     * This is the normal code. That is, the case when binops are done as
     * an in-line switch in exec(). The other case is when that blows
     * your compiler away and it is seperated out as a function.
     */
    o = new_op(nullptr, why == FOR_TEMP ? OP_BINOP_FOR_TEMP : OP_BINOP, op);
#else
    o = new_op(op_binop, 0, op);
#endif
    if (o == nullptr)
    {
        return nullptr;
    }
    if (why != FOR_TEMP)
    {
        binops[op] = o;
    }
    else
    {
        binops_temps[op] = o;
    }
    return o;
}

/*
 * Compile the expression into the code array, for the reason given.
 * Returns 1 on failure, 0 on success.
 */
int compile_expr(array *a, expr *e, int why)
{

#define NOTLV(why) ((why) == FOR_LVALUE ? FOR_VALUE : (why))
#define NOTTEMP(why) ((why) == FOR_TEMP ? FOR_VALUE : (why))

    if (a->push_check())
    {
        return 1;
    }
    if (t_type(e->e_what) == T_BINOP && e->e_arg[1] != nullptr)
    {
        if (e->e_what == T_COMMA)
        {
            if (compile_expr(a, e->e_arg[0], FOR_EFFECT))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], why))
            {
                return 1;
            }
            return 0;
        }
        if (e->e_what == T_QUESTION)
        {
            ref<array> a1;
            ref<array> a2;

            if (e->e_arg[1]->e_what != T_COLON)
            {
                return set_error("syntax error in \"? :\" use");
            }
            if (compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = new_array()) == nullptr)
            {
                return 1;
            }
            if (compile_expr(a1, e->e_arg[1]->e_arg[0], why) || a1->push_check())
            {
                return 1;
            }
            a1->push(&o_end);
            if ((a2 = new_array()) == nullptr)
            {
                return 1;
            }
            if (compile_expr(a2, e->e_arg[1]->e_arg[1], why) || a2->push_check() || a->push_check(3))
            {
                return 1;
            }
            a2->push(&o_end);
            a->push(&o_ifelse);
            a->push(a1.release(with_decref));
            a->push(a2.release(with_decref));
            return 0;
        }
        if (e->e_what == T_LESSEQGRT)
        {
            if (compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], FOR_LVALUE))
            {
                return 1;
            }
            if (a->push_check())
            {
                return 1;
            }
            auto o = new_op(nullptr, OP_SWAP, NOTTEMP(why));
            if (!o)
            {
                return 1;
            }
            a->push(o, with_decref);
            return 0;
        }
        if (e->e_what == T_EQ || e->e_what == T_COLONEQ)
        {
            /*
             * Simple assignment.
             */
            if (e->e_arg[0]->e_what == T_NAME && e->e_what == T_COLONEQ)
            {
                if (a->push_check(2))
                {
                    return 1;
                }
                a->push(&o_quote);
                a->push(e->e_arg[0]->e_obj);
                if (compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (a->push_check())
                {
                    return 1;
                }
                auto o = new_op(nullptr, OP_ASSIGNLOCALVAR, NOTTEMP(why));
                if (!o)
                {
                    return 1;
                }
                a->push(o, with_decref);
                return 0;
            }
            if (e->e_arg[0]->e_what == T_NAME)
            {
                if (compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (a->push_check(2))
                {
                    return 1;
                }
                auto o = new_op(nullptr, OP_ASSIGN_TO_NAME, NOTTEMP(why));
                if (!o)
                {
                    return 1;
                }
                a->push(o, with_decref);
                a->push(e->e_arg[0]->e_obj);
                return 0;
            }
            if (compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], FOR_VALUE))
            {
                return 1;
            }
            if (a->push_check())
            {
                return 1;
            }
            auto o = new_op(nullptr, e->e_what == T_EQ ? OP_ASSIGN : OP_ASSIGNLOCAL, NOTTEMP(why));
            if (!o)
            {
                return 1;
            }
            a->push(o, with_decref);
            return 0;
        }
        if (e->e_what >= T_EQ)
        {
            /*
             * Assignment op.
             */
            if (compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (a->push_check())
            {
                return 1;
            }
            a->push(&o_dotkeep);
            if (compile_expr(a, e->e_arg[1], FOR_TEMP))
            {
                return 1;
            }
            if (a->push_check(2))
            {
                return 1;
            }
            auto o = new_binop(e->e_what, FOR_VALUE);
            if (!o)
            {
                return 1;
            }
            a->push(o);
            o = new_op(nullptr, OP_ASSIGN, NOTTEMP(why));
            if (!o)
            {
                return 1;
            }
            a->push(o, with_decref);
            return 0;
        }
        if (why == FOR_LVALUE)
        {
            goto notlvalue;
        }
        if (e->e_what == T_ANDAND || e->e_what == T_BARBAR)
        {
            array *a1;

            if (compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = new_array()) == nullptr)
            {
                return 1;
            }
            if (compile_expr(a1, e->e_arg[1], FOR_VALUE) || a1->push_check(3) || a->push_check(3))
            {
                decref(a1);
                return 1;
            }
            a1->push(&o_end);
            a->push(a1, with_decref);
            a->push(e->e_what == T_ANDAND ? &o_andand : &o_barbar);
            if (why == FOR_EFFECT)
            {
                a->push(&o_pop);
            }
            return 0;
        }
        /*
         * Ordinary binary op. All binary operators that take an int or a float
         * on either side can take a temp.
         */
        if (compile_expr(a, e->e_arg[0], why == FOR_VALUE ? FOR_TEMP : why))
        {
            return 1;
        }
        if (compile_expr(a, e->e_arg[1], why == FOR_VALUE ? FOR_TEMP : why))
        {
            return 1;
        }
        if (why == FOR_EFFECT)
        {
            return 0;
        }
        if (a->push_check())
        {
            return 1;
        }
        if (auto o = new_binop(e->e_what, why))
        {
            a->push(o);
            return 0;
        }
        return 1;
    }
    else
    {
        object *op1, *op2;

        /*
         * Not a "binary operator".
         */
        if (a->push_check(3)) /* Worst case below. */
        {
            return 1;
        }
        switch (e->e_what)
        {
        case T_NULL:
            if (why != FOR_EFFECT)
            {
                a->push(null);
            }
            break;

        case T_DOLLAR: {
            array *a1;

            if ((a1 = new_array()) == nullptr)
            {
                return 1;
            }
            if (why == FOR_TEMP)
            {
                why = FOR_VALUE;
            }
            if (compile_expr(a1, e->e_arg[0], NOTLV(why)) || a1->push_check())
            {
                decref(a1);
                return 1;
            }
            a1->push(&o_end);
            if ((e->e_obj = evaluate(a1, 0)) == nullptr)
            {
                decref(a1);
                return 1;
            }
            decref(a1);
        }
            /* Fall through. */
        case T_INT:
        case T_FLOAT:
        case T_CONST:
        case T_STRING:
            if (why == FOR_LVALUE)
            {
                goto notlvalue;
            }
            if (why != FOR_EFFECT)
            {
                if (isstring(e->e_obj))
                {
                    a->push(&o_quote);
                }
                a->push(e->e_obj);
            }
            break;

        case T_NAME:
            if (why == FOR_LVALUE)
            {
                a->push(&o_namelvalue);
            }
            a->push(e->e_obj);
            if (why == FOR_EFFECT)
            {
                /*
                 * We do evaluate variables even if only for value, because
                 * they can have the side-effect of loading a module. But we
                 * then have to pop the value.
                 */
                a->push(&o_pop);
            }
            return 0;

        case T_PLUS:
            if (compile_expr(a, e->e_arg[0], NOTLV(why)))
            {
                return 1;
            }
            break;

        case T_PLUSPLUS:
        case T_MINUSMINUS:
            if (e->e_arg[0] == nullptr)
            {
                /*
                 * Postfix.
                 */
                if (compile_expr(a, e->e_arg[1], FOR_LVALUE))
                {
                    return 1;
                }
                if (why == FOR_EFFECT)
                {
                    goto pluspluseffect;
                }

                if (a->push_check(4))
                {
                    return 1;
                }
                a->push(&o_dotrkeep);
                a->push(o_one);
                {
                    op1 = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE);
                    if (!op1)
                    {
                        return 1;
                    }
                    a->push(op1);
                    op2 = new_op(nullptr, OP_ASSIGN, FOR_EFFECT);
                    if (!op2)
                    {
                        return 1;
                    }
                    a->push(op2, with_decref);
                }
            }
            else
            {
                /*
                 * Prefix, (or possibly postfix for effect).
                 */
                if (compile_expr(a, e->e_arg[0], FOR_LVALUE))
                {
                    return 1;
                }
pluspluseffect:
                if (a->push_check(4))
                {
                    return 1;
                }
                a->push(&o_dotkeep);
                a->push(o_one);
                op1 = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE);
                if (!op1)
                {
                    return 1;
                }
                a->push(op1);
                op2 = new_op(nullptr, OP_ASSIGN, NOTTEMP(why));
                if (!op2)
                {
                    return 1;
                }
                a->push(op2, with_decref);
                return 0;
            }
            break;

        case T_MINUS:
            /*
             * We implement unary minus as 0 - x because the binary arithmetic
             * code is more efficient.
             */
            if (why == FOR_EFFECT)
            {
                return compile_expr(a, e->e_arg[0], FOR_EFFECT);
            }
            a->push(o_zero);
            if (compile_expr(a, e->e_arg[0], NOTLV(why)))
            {
                return 1;
            }
            if (a->push_check())
            {
                return 1;
            }
            op1 = new_binop(e->e_what, why);
            if (!op1)
            {
                break;
            }
            a->push(op1);
            break;

        case T_TILDE:
            if (compile_expr(a, e->e_arg[0], why == FOR_VALUE ? FOR_TEMP : NOTLV(why)))
            {
                return 1;
            }
            goto unary_arith;
        case T_EXCLAM:
            if (compile_expr(a, e->e_arg[0], why == FOR_TEMP ? FOR_VALUE : NOTLV(why)))
            {
                return 1;
            }
unary_arith:
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (a->push_check())
            {
                return 1;
            }
            op1 = new_op(op_unary, 0, t_subtype(e->e_what));
            if (!op1)
            {
                return 1;
            }
            a->push(op1, with_decref);
            break;

        case T_AT:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (a->push_check())
            {
                return 1;
            }
            op1 = new_op(nullptr, OP_AT, 0);
            if (!op1)
            {
                return 1;
            }
            a->push(op1, with_decref);
            break;

        case T_AND: /* Unary. */
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_LVALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (a->push_check())
            {
                return 1;
            }
            a->push(&o_mkptr);
            break;

        case T_ASTERIX: /* Unary. */
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (a->push_check())
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                a->push(&o_openptr);
                return 0;
            }
            else
            {
                a->push(&o_fetch);
            }
            break;

        case T_ONSQUARE: /* Array or pointer index. */
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (a->push_check())
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                return 0;
            }
            a->push(&o_dot);
            break;

        case T_PRIMARYCOLON:
        case T_COLONCARET:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (why == FOR_LVALUE)
            {
                goto notlvalue;
            }
            if (a->push_check())
            {
                return 1;
            }
            op1 = new_op(nullptr, OP_COLON, e->e_what == T_COLONCARET ? OPC_COLON_CARET : 0);
            if (!op1)
            {
                return 1;
            }
            a->push(op1, with_decref);
            break;

        case T_PTR:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why != FOR_EFFECT)
            {
                if (a->push_check())
                {
                    return 1;
                }
                a->push(&o_fetch);
            }
            goto dot2;
        case T_DOT:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
dot2:
            if (compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (why == FOR_LVALUE)
            {
                return 0;
            }
            if (a->push_check())
            {
                return 1;
            }
            a->push(&o_dot);
            break;

        case T_BINAT:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (why == FOR_LVALUE)
            {
                goto notlvalue;
            }
            if (a->push_check())
            {
                return 1;
            }
            a->push(&o_mkptr);
            break;

        case T_ONROUND: /* Function call. */
        {
            int   nargs;
            expr *e1;

            /*
             * First, code the arguments. Evaluated in right to
             * left order. Also code the number of actuals.
             */
            nargs = 0;
            for (e1 = e->e_arg[1]; e1 != nullptr; e1 = e1->e_arg[1])
            {
                if (compile_expr(a, e1->e_arg[0], FOR_VALUE))
                {
                    return 1;
                }
                ++nargs;
            }
            if (a->push_check())
            {
                return 1;
            }
            if ((*a->a_top = new_int(nargs)) == nullptr)
            {
                return 1;
            }
            decref((*a->a_top));
            ++a->a_top;
            if (e->e_arg[0]->e_what == T_PRIMARYCOLON || e->e_arg[0]->e_what == T_COLONCARET)
            {
                /*
                 * Special optimisation. The call is of the form
                 *
                 *   this:that(...) or this:^that()
                 *
                 * Use the direct method call to avoid ever forming
                 * the method object.
                 */
                if (compile_expr(a, e->e_arg[0]->e_arg[0], FOR_VALUE))
                {
                    return 1;
                }
                if (compile_expr(a, e->e_arg[0]->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (a->push_check(2))
                {
                    return 1;
                }
                if (e->e_arg[0]->e_what == T_COLONCARET)
                {
                    a->push(&o_super_call);
                }
                else
                {
                    a->push(&o_method_call);
                }
            }
            else
            {
                /*
                 * Normal case. Code the thing being called and a call
                 * operation.
                 */
                if (compile_expr(a, e->e_arg[0], FOR_VALUE))
                {
                    return 1;
                }
                if (a->push_check(2))
                {
                    return 1;
                }
                a->push(&o_call);
            }
            if (why == FOR_EFFECT)
            {
                a->push(&o_pop);
            }
        }
        break;
        }
    }
    if (why == FOR_LVALUE)
    {
        if (a->push_check())
        {
            return 1;
        }
        a->push(&o_mklvalue);
    }
    return 0;

notlvalue:
    return set_error("lvalue required");
}

/*
 * Destroys static information created in this file.
 */
void uninit_compile()
{
    int i;

    for (i = 0; i <= BINOP_MAX; ++i)
    {
        if (binops[i] != nullptr)
        {
            decref(binops[i]);
            binops[i] = nullptr;
        }
        if (binops_temps[i] != nullptr)
        {
            decref(binops_temps[i]);
            binops_temps[i] = nullptr;
        }
    }
}

} // namespace ici

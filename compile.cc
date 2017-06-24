#define ICI_CORE
#include "fwd.h"
#include "parse.h"
#include "array.h"
#include "op.h"
#include "null.h"
#include "int.h"
#include "str.h"

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
static object *
new_binop(int op, int why)
{
    object           *o;

    op = t_subtype(op);
    if (why != FOR_TEMP)
    {
        if ((o = binops[op]) != NULL)
        {
            return o;
        }
    }
    else
    {
        if ((o = binops_temps[op]) != NULL)
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
    o = ici_new_op(NULL, why == FOR_TEMP ? OP_BINOP_FOR_TEMP : OP_BINOP, op);
#else
    o = ici_new_op(op_binop, 0, op);
#endif
    if (o == NULL)
    {
        return NULL;
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

#define NOTLV(why)      ((why) == FOR_LVALUE ? FOR_VALUE : (why))
#define NOTTEMP(why)    ((why) == FOR_TEMP ? FOR_VALUE : (why))

    if (a->stk_push_chk())
    {
        return 1;
    }
    if (t_type(e->e_what) == T_BINOP && e->e_arg[1] != NULL)
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
            array *a1;
            array *a2;

            if (e->e_arg[1]->e_what != T_COLON)
            {
                return set_error("syntax error in \"? :\" use");
            }
            if (compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return 1;
            }
            if (compile_expr(a1, e->e_arg[1]->e_arg[0], why) || a1->stk_push_chk())
            {
                a1->decref();
                return 1;
            }
            *a1->a_top++ = &o_end;
            if ((a2 = ici_array_new(0)) == NULL)
            {
                a1->decref();
                return 1;
            }
            if
            (
                compile_expr(a2, e->e_arg[1]->e_arg[1], why)
                ||
                a2->stk_push_chk()
                ||
                a->stk_push_chk(3)
            )
            {
                a1->decref();
                a2->decref();
                return 1;
            }
            *a2->a_top++ = &o_end;
            *a->a_top++ = &o_ifelse;
            *a->a_top++ = a1;
            *a->a_top++ = a2;
            a1->decref();
            a2->decref();
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, OP_SWAP, NOTTEMP(why))) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            a->a_top++;
            return 0;
        }
        if (e->e_what == T_EQ || e->e_what == T_COLONEQ)
        {
            /*
             * Simple assignment.
             */
            if (e->e_arg[0]->e_what == T_NAME && e->e_what == T_COLONEQ)
            {
                if (a->stk_push_chk(2))
                {
                    return 1;
                }
                *a->a_top++ = &o_quote;
                *a->a_top++ = e->e_arg[0]->e_obj;
                if (compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (a->stk_push_chk())
                {
                    return 1;
                }
                if ((*a->a_top = ici_new_op(NULL, OP_ASSIGNLOCALVAR, NOTTEMP(why))) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                a->a_top++;
                return 0;
            }
            if (e->e_arg[0]->e_what == T_NAME)
            {
                if (compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (a->stk_push_chk(2))
                {
                    return 1;
                }
                if ((*a->a_top = ici_new_op(NULL, OP_ASSIGN_TO_NAME, NOTTEMP(why))) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                ++a->a_top;
                *a->a_top++ = e->e_arg[0]->e_obj;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, e->e_what == T_EQ ? OP_ASSIGN : OP_ASSIGNLOCAL, NOTTEMP(why))) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            a->a_top++;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            *a->a_top++ = &o_dotkeep;
            if (compile_expr(a, e->e_arg[1], FOR_TEMP))
            {
                return 1;
            }
            if (a->stk_push_chk(2))
            {
                return 1;
            }
            if ((*a->a_top = new_binop(e->e_what, FOR_VALUE)) == NULL)
            {
                return 1;
            }
            ++a->a_top;
            if ((*a->a_top = ici_new_op(NULL, OP_ASSIGN, NOTTEMP(why))) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            a->a_top++;
            return 0;
        }
        if (why == FOR_LVALUE)
        {
            goto notlvalue;
        }
        if (e->e_what == T_ANDAND || e->e_what == T_BARBAR)
        {
            array    *a1;

            if (compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return 1;
            }
            if
            (
                compile_expr(a1, e->e_arg[1], FOR_VALUE)
                ||
                a1->stk_push_chk(3)
                ||
                a->stk_push_chk(3)
            )
            {
                a1->decref();
                return 1;
            }
            *a1->a_top++ = &o_end;
            *a->a_top++ = a1;
            a1->decref();
            *a->a_top++ = e->e_what == T_ANDAND ? &o_andand : &o_barbar;
            if (why == FOR_EFFECT)
            {
                *a->a_top++ = &o_pop;
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
        if (a->stk_push_chk())
        {
            return 1;
        }
        if (why == FOR_EFFECT)
        {
            return 0;
        }
        if ((*a->a_top = new_binop(e->e_what, why)) == NULL)
        {
            return 1;
        }
        ++a->a_top;
        return 0;
    }
    else
    {
        /*
         * Not a "binary opertor".
         */
        if (a->stk_push_chk(3)) /* Worst case below. */
        {
            return 1;
        }
        switch (e->e_what)
        {
        case T_NULL:
            if (why != FOR_EFFECT)
            {
                *a->a_top++ = ici_null;
            }
            break;

        case T_DOLLAR:
            {
                array *a1;

                if ((a1 = ici_array_new(0)) == NULL)
                {
                    return 1;
                }
                if (why == FOR_TEMP)
                {
                    why = FOR_VALUE;
                }
                if
                (
                    compile_expr(a1, e->e_arg[0], NOTLV(why))
                    ||
                    a1->stk_push_chk()
                )
                {
                    a1->decref();
                    return 1;
                }
                *a1->a_top++ = &o_end;
                if ((e->e_obj = evaluate(a1, 0)) == NULL)
                {
                    a1->decref();
                    return 1;
                }
                a1->decref();
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
                    *a->a_top++ = &o_quote;
                }
                *a->a_top++ = e->e_obj;
            }
            break;

        case T_NAME:
            if (why == FOR_LVALUE)
            {
                *a->a_top++ = &o_namelvalue;
            }
            *a->a_top++ = e->e_obj;
            if (why == FOR_EFFECT)
            {
                /*
                 * We do evaluate variables even if only for value, because
                 * they can have the side-effect of loading a module. But we
                 * then have to pop the value.
                 */
                *a->a_top++ = &o_pop;
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
            if (e->e_arg[0] == NULL)
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

                if (a->stk_push_chk(4))
                {
                    return 1;
                }
                *a->a_top++ = &o_dotrkeep;
                *a->a_top++ = o_one;
                if ((*a->a_top = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE)) == NULL)
                {
                    return 1;
                }
                ++a->a_top;
                if ((*a->a_top = ici_new_op(NULL, OP_ASSIGN, FOR_EFFECT)) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                a->a_top++;
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
                if (a->stk_push_chk(4))
                {
                    return 1;
                }
                *a->a_top++ = &o_dotkeep;
                *a->a_top++ = o_one;
                if ((*a->a_top = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE)) == NULL)
                {
                    return 1;
                }
                ++a->a_top;
                if ((*a->a_top = ici_new_op(NULL, OP_ASSIGN, NOTTEMP(why))) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                a->a_top++;
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
            *a->a_top++ = o_zero;
            if (compile_expr(a, e->e_arg[0], NOTLV(why)))
            {
                return 1;
            }
            if (a->stk_push_chk())
            {
                return 1;
            }
            if ((*a->a_top = new_binop(e->e_what, why)) == NULL)
            {
                break;
            }
            ++a->a_top;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(op_unary, 0, t_subtype(e->e_what))) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, OP_AT, 0)) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            *a->a_top++ = &o_mkptr;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                *a->a_top++ = &o_openptr;
                return 0;
            }
            else
            {
                *a->a_top++ = &o_fetch;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                return 0;
            }
            *a->a_top++ = &o_dot;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            *a->a_top = ici_new_op
                (
                    NULL,
                    OP_COLON,
                    e->e_what == T_COLONCARET ? OPC_COLON_CARET : 0
                );
            if (*a->a_top == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            a->a_top++;
            break;

        case T_PTR:
            if (compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why != FOR_EFFECT)
            {
                if (a->stk_push_chk())
                {
                    return 1;
                }
                *a->a_top++ = &o_fetch;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            *a->a_top++ = &o_dot;
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
            if (a->stk_push_chk())
            {
                return 1;
            }
            *a->a_top++ = &o_mkptr;
            break;

        case T_ONROUND: /* Function call. */
            {
                int   nargs;
                expr  *e1;

                /*
                 * First, code the arguments. Evaluated in right to
                 * left order. Also code the number of actuals.
                 */
                nargs = 0;
                for (e1 = e->e_arg[1]; e1 != NULL; e1 = e1->e_arg[1])
                {
                    if (compile_expr(a, e1->e_arg[0], FOR_VALUE))
                    {
                        return 1;
                    }
                    ++nargs;
                }
                if (a->stk_push_chk())
                {
                    return 1;
                }
                if ((*a->a_top = ici_int_new(nargs)) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                ++a->a_top;
                if
                (
                    e->e_arg[0]->e_what == T_PRIMARYCOLON
                    ||
                    e->e_arg[0]->e_what == T_COLONCARET
                )
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
                    if (a->stk_push_chk(2))
                    {
                        return 1;
                    }
                    if (e->e_arg[0]->e_what == T_COLONCARET)
                    {
                        *a->a_top++ = &o_super_call;
                    }
                    else
                    {
                        *a->a_top++ = &o_method_call;
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
                    if (a->stk_push_chk(2))
                    {
                        return 1;
                    }
                    *a->a_top++ = &o_call;
                }
                if (why == FOR_EFFECT)
                {
                    *a->a_top++ = &o_pop;
                }
            }
            break;
        }
    }
    if (why == FOR_LVALUE)
    {
        if (a->stk_push_chk())
        {
            return 1;
        }
        *a->a_top++ = &o_mklvalue;
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

    for (i = 0; i <= BINOP_MAX; ++ i)
    {
        if (binops[i] != NULL)
        {
            binops[i]->decref();
            binops[i] = NULL;
        }
        if (binops_temps[i] != NULL)
        {
            binops_temps[i]->decref();
            binops_temps[i] = NULL;
        }
    }
}

} // namespace ici

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
static ici_obj_t *binops[BINOP_MAX + 1];
static ici_obj_t *binops_temps[BINOP_MAX + 1];

/*
 * Return a new op object corresponding to the binary operation of the
 * given token, which is a binary operator. The returned object does
 * not have an extra reference (unlike most new_* functions).
 */
static ici_obj_t *
new_binop(int op, int why)
{
    ici_obj_t           *o;

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
    o = ici_new_op(NULL, why == FOR_TEMP ? ICI_OP_BINOP_FOR_TEMP : ICI_OP_BINOP, op);
#else
    o = ici_new_op(ici_op_binop, 0, op);
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
int
ici_compile_expr(ici_array_t *a, expr_t *e, int why)
{

#define NOTLV(why)      ((why) == FOR_LVALUE ? FOR_VALUE : (why))
#define NOTTEMP(why)    ((why) == FOR_TEMP ? FOR_VALUE : (why))

    if (ici_stk_push_chk(a, 1))
    {
        return 1;
    }
    if (t_type(e->e_what) == T_BINOP && e->e_arg[1] != NULL)
    {
        if (e->e_what == T_COMMA)
        {
            if (ici_compile_expr(a, e->e_arg[0], FOR_EFFECT))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], why))
            {
                return 1;
            }
            return 0;
        }
        if (e->e_what == T_QUESTION)
        {
            ici_array_t *a1;
            ici_array_t *a2;

            if (e->e_arg[1]->e_what != T_COLON)
            {
                return ici_set_error("syntax error in \"? :\" use");
            }
            if (ici_compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return 1;
            }
            if (ici_compile_expr(a1, e->e_arg[1]->e_arg[0], why) || ici_stk_push_chk(a1, 1))
            {
                a1->decref();
                return 1;
            }
            *a1->a_top++ = &ici_o_end;
            if ((a2 = ici_array_new(0)) == NULL)
            {
                a1->decref();
                return 1;
            }
            if
            (
                ici_compile_expr(a2, e->e_arg[1]->e_arg[1], why)
                ||
                ici_stk_push_chk(a2, 1)
                ||
                ici_stk_push_chk(a, 3)
            )
            {
                a1->decref();
                a2->decref();
                return 1;
            }
            *a2->a_top++ = &ici_o_end;
            *a->a_top++ = &ici_o_ifelse;
            *a->a_top++ = a1;
            *a->a_top++ = a2;
            a1->decref();
            a2->decref();
            return 0;
        }
        if (e->e_what == T_LESSEQGRT)
        {
            if (ici_compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], FOR_LVALUE))
            {
                return 1;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, ICI_OP_SWAP, NOTTEMP(why))) == NULL)
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
                if (ici_stk_push_chk(a, 2))
                {
                    return 1;
                }
                *a->a_top++ = &ici_o_quote;
                *a->a_top++ = e->e_arg[0]->e_obj;
                if (ici_compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (ici_stk_push_chk(a, 1))
                {
                    return 1;
                }
                if ((*a->a_top = ici_new_op(NULL, ICI_OP_ASSIGNLOCALVAR, NOTTEMP(why))) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                a->a_top++;
                return 0;
            }
            if (e->e_arg[0]->e_what == T_NAME)
            {
                if (ici_compile_expr(a, e->e_arg[1], FOR_VALUE))
                {
                    return 1;
                }
                if (ici_stk_push_chk(a, 2))
                {
                    return 1;
                }
                if ((*a->a_top = ici_new_op(NULL, ICI_OP_ASSIGN_TO_NAME, NOTTEMP(why))) == NULL)
                {
                    return 1;
                }
                (*a->a_top)->decref();
                ++a->a_top;
                *a->a_top++ = e->e_arg[0]->e_obj;
                return 0;
            }
            if (ici_compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], FOR_VALUE))
            {
                return 1;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, e->e_what == T_EQ ? ICI_OP_ASSIGN : ICI_OP_ASSIGNLOCAL, NOTTEMP(why))) == NULL)
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
            if (ici_compile_expr(a, e->e_arg[0], FOR_LVALUE))
            {
                return 1;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            *a->a_top++ = &ici_o_dotkeep;
            if (ici_compile_expr(a, e->e_arg[1], FOR_TEMP))
            {
                return 1;
            }
            if (ici_stk_push_chk(a, 2))
            {
                return 1;
            }
            if ((*a->a_top = new_binop(e->e_what, FOR_VALUE)) == NULL)
            {
                return 1;
            }
            ++a->a_top;
            if ((*a->a_top = ici_new_op(NULL, ICI_OP_ASSIGN, NOTTEMP(why))) == NULL)
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
            ici_array_t    *a1;

            if (ici_compile_expr(a, e->e_arg[0], FOR_VALUE))
            {
                return 1;
            }
            if ((a1 = ici_array_new(0)) == NULL)
            {
                return 1;
            }
            if
            (
                ici_compile_expr(a1, e->e_arg[1], FOR_VALUE)
                ||
                ici_stk_push_chk(a1, 3)
                ||
                ici_stk_push_chk(a, 3)
            )
            {
                a1->decref();
                return 1;
            }
            *a1->a_top++ = &ici_o_end;
            *a->a_top++ = a1;
            a1->decref();
            *a->a_top++ = e->e_what == T_ANDAND ? &ici_o_andand : &ici_o_barbar;
            if (why == FOR_EFFECT)
            {
                *a->a_top++ = &ici_o_pop;
            }
            return 0;
        }
        /*
         * Ordinary binary op. All binary operators that take an int or a float
         * on either side can take a temp.
         */
        if (ici_compile_expr(a, e->e_arg[0], why == FOR_VALUE ? FOR_TEMP : why))
        {
            return 1;
        }
        if (ici_compile_expr(a, e->e_arg[1], why == FOR_VALUE ? FOR_TEMP : why))
        {
            return 1;
        }
        if (ici_stk_push_chk(a, 1))
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
        if (ici_stk_push_chk(a, 3)) /* Worst case below. */
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
                ici_array_t *a1;

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
                    ici_compile_expr(a1, e->e_arg[0], NOTLV(why))
                    ||
                    ici_stk_push_chk(a1, 1)
                )
                {
                    a1->decref();
                    return 1;
                }
                *a1->a_top++ = &ici_o_end;
                if ((e->e_obj = ici_evaluate(a1, 0)) == NULL)
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
                if (ici_isstring(e->e_obj))
                {
                    *a->a_top++ = &ici_o_quote;
                }
                *a->a_top++ = e->e_obj;
            }
            break;

        case T_NAME:
            if (why == FOR_LVALUE)
            {
                *a->a_top++ = &ici_o_namelvalue;
            }
            *a->a_top++ = e->e_obj;
            if (why == FOR_EFFECT)
            {
                /*
                 * We do evaluate variables even if only for value, because
                 * they can have the side-effect of loading a module. But we
                 * then have to pop the value.
                 */
                *a->a_top++ = &ici_o_pop;
            }
            return 0;

        case T_PLUS:
            if (ici_compile_expr(a, e->e_arg[0], NOTLV(why)))
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
                if (ici_compile_expr(a, e->e_arg[1], FOR_LVALUE))
                {
                    return 1;
                }
                if (why == FOR_EFFECT)
                {
                    goto pluspluseffect;
                }

                if (ici_stk_push_chk(a, 4))
                {
                    return 1;
                }
                *a->a_top++ = &ici_o_dotrkeep;
                *a->a_top++ = ici_one;
                if ((*a->a_top = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE)) == NULL)
                {
                    return 1;
                }
                ++a->a_top;
                if ((*a->a_top = ici_new_op(NULL, ICI_OP_ASSIGN, FOR_EFFECT)) == NULL)
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
                if (ici_compile_expr(a, e->e_arg[0], FOR_LVALUE))
                {
                    return 1;
                }
            pluspluseffect:
                if (ici_stk_push_chk(a, 4))
                {
                    return 1;
                }
                *a->a_top++ = &ici_o_dotkeep;
                *a->a_top++ = ici_one;
                if ((*a->a_top = new_binop(e->e_what == T_PLUSPLUS ? T_PLUS : T_MINUS, FOR_VALUE)) == NULL)
                {
                    return 1;
                }
                ++a->a_top;
                if ((*a->a_top = ici_new_op(NULL, ICI_OP_ASSIGN, NOTTEMP(why))) == NULL)
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
                return ici_compile_expr(a, e->e_arg[0], FOR_EFFECT);
            }
            *a->a_top++ = ici_zero;
            if (ici_compile_expr(a, e->e_arg[0], NOTLV(why)))
            {
                return 1;
            }
            if (ici_stk_push_chk(a, 1))
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
            if (ici_compile_expr(a, e->e_arg[0], why == FOR_VALUE ? FOR_TEMP : NOTLV(why)))
            {
                return 1;
            }
            goto unary_arith;
        case T_EXCLAM:
            if (ici_compile_expr(a, e->e_arg[0], why == FOR_TEMP ? FOR_VALUE : NOTLV(why)))
            {
                return 1;
            }
        unary_arith:
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(ici_op_unary, 0, t_subtype(e->e_what))) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
            break;

        case T_AT:
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if ((*a->a_top = ici_new_op(NULL, ICI_OP_AT, 0)) == NULL)
            {
                return 1;
            }
            (*a->a_top)->decref();
            ++a->a_top;
            break;

        case T_AND: /* Unary. */
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_LVALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            *a->a_top++ = &ici_o_mkptr;
            break;

        case T_ASTERIX: /* Unary. */
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                *a->a_top++ = &ici_o_openptr;
                return 0;
            }
            else
            {
                *a->a_top++ = &ici_o_fetch;
            }
            break;

        case T_ONSQUARE: /* Array or pointer index. */
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why == FOR_EFFECT)
            {
                break;
            }
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            if (why == FOR_LVALUE)
            {
                return 0;
            }
            *a->a_top++ = &ici_o_dot;
            break;

        case T_PRIMARYCOLON:
        case T_COLONCARET:
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
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
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            *a->a_top = ici_new_op
                (
                    NULL,
                    ICI_OP_COLON,
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
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (why != FOR_EFFECT)
            {
                if (ici_stk_push_chk(a, 1))
                {
                    return 1;
                }
                *a->a_top++ = &ici_o_fetch;
            }
            goto dot2;
        case T_DOT:
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
        dot2:
            if (ici_compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
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
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            *a->a_top++ = &ici_o_dot;
            break;

        case T_BINAT:
            if (ici_compile_expr(a, e->e_arg[0], why != FOR_EFFECT ? FOR_VALUE : why))
            {
                return 1;
            }
            if (ici_compile_expr(a, e->e_arg[1], why != FOR_EFFECT ? FOR_VALUE : why))
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
            if (ici_stk_push_chk(a, 1))
            {
                return 1;
            }
            *a->a_top++ = &ici_o_mkptr;
            break;

        case T_ONROUND: /* Function call. */
            {
                int     nargs;
                expr_t  *e1;

                /*
                 * First, code the arguments. Evaluated in right to
                 * left order. Also code the number of actuals.
                 */
                nargs = 0;
                for (e1 = e->e_arg[1]; e1 != NULL; e1 = e1->e_arg[1])
                {
                    if (ici_compile_expr(a, e1->e_arg[0], FOR_VALUE))
                    {
                        return 1;
                    }
                    ++nargs;
                }
                if (ici_stk_push_chk(a, 1))
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
                    if (ici_compile_expr(a, e->e_arg[0]->e_arg[0], FOR_VALUE))
                    {
                        return 1;
                    }
                    if (ici_compile_expr(a, e->e_arg[0]->e_arg[1], FOR_VALUE))
                    {
                        return 1;
                    }
                    if (ici_stk_push_chk(a, 2))
                    {
                        return 1;
                    }
                    if (e->e_arg[0]->e_what == T_COLONCARET)
                    {
                        *a->a_top++ = &ici_o_super_call;
                    }
                    else
                    {
                        *a->a_top++ = &ici_o_method_call;
                    }
                }
                else
                {
                    /*
                     * Normal case. Code the thing being called and a call
                     * operation.
                     */
                    if (ici_compile_expr(a, e->e_arg[0], FOR_VALUE))
                    {
                        return 1;
                    }
                    if (ici_stk_push_chk(a, 2))
                    {
                        return 1;
                    }
                    *a->a_top++ = &ici_o_call;
                }
                if (why == FOR_EFFECT)
                {
                    *a->a_top++ = &ici_o_pop;
                }
            }
            break;
        }
    }
    if (why == FOR_LVALUE)
    {
        if (ici_stk_push_chk(a, 1))
        {
            return 1;
        }
        *a->a_top++ = &ici_o_mklvalue;
    }
    return 0;

notlvalue:
    return ici_set_error("lvalue required");
}

/*
 * Destroys static information created in this file.
 */
void
ici_uninit_compile()
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

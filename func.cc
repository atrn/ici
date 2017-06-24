#define ICI_CORE
#include "fwd.h"
#include "func.h"
#include "exec.h"
#include "ptr.h"
#include "struct.h"
#include "op.h"
#include "pc.h"
#include "src.h"
#include "str.h"
#include "catch.h"
#include "buf.h"
#include "mark.h"
#include "null.h"
#include "primes.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

namespace ici
{

func *
ici_new_func()
{
    func *f;

    if ((f = ici_talloc(func)) == NULL)
    {
        return NULL;
    }
    memset((char *)f, 0, sizeof(ici_func_t));
    ICI_OBJ_SET_TFNZ(f, ICI_TC_FUNC, 0, 1, 0);
    ici_rego(f);
    return f;
}

int
ici_op_return()
{
    object              **x;
    static int          occasionally;
    ici_obj_t           *f;

    if (UNLIKELY(ici_debug_active))
    {
        debugfunc->idbg_fnresult(ici_os.a_top[-1]);
    }

    x = ici_xs.a_top - 1;
    while
    (
        !ici_ismark(*x)
        &&
        --x >= ici_xs.a_base
        &&
        !(ici_iscatch(*x) && isnull(ici_catchof(*x)->c_catcher))
    )
        ;
    if (x < ici_xs.a_base || !ici_ismark(*x))
    {
        return ici_set_error("return not in function");
    }
    ici_xs.a_top = x;

    /*
     * If convenient, record the total nels of the autos that this function
     * ended up with, as a hint for the auto struct allocation on next call.
     * If it isn't convenient, do it occasionally anyway.
     */
    if
    (
        SS(_func_)->s_struct == ici_vs.a_top[-1]
        &&
        SS(_func_)->s_vsver == vsver
        &&
        ici_isfunc(f = SS(_func_)->s_slot->sl_value)
    )
    {
        ici_funcof(f)->f_nautos = ici_structof(ici_vs.a_top[-1])->s_nels;
    }
    else if (--occasionally <= 0)
    {
        occasionally = 10;
        f = ici_fetch(ici_vs.a_top[-1], SS(_func_));
        if (ici_isstruct(ici_vs.a_top[-1]) && ici_isfunc(f))
            ici_funcof(f)->f_nautos = ici_structof(ici_vs.a_top[-1])->s_nels;
    }

    --ici_vs.a_top;
#ifndef NOPROFILE
    if (ici_profile_active)
        ici_profile_return();
#endif
    return 0;
}

size_t func_type::mark(ici_obj_t *o)
{
    auto fn = ici_funcof(o);
    o->setmark();
    auto mem = typesize();
    if (fn->f_code != NULL)
        mem += ici_mark(fn->f_code);
    if (fn->f_args != NULL)
        mem += ici_mark(fn->f_args);
    if (fn->f_autos != NULL)
        mem += ici_mark(fn->f_autos);
    if (fn->f_name != NULL)
        mem += ici_mark(fn->f_name);
    return mem;
}

int func_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    return ici_funcof(o1)->f_code != ici_funcof(o2)->f_code
        || ici_funcof(o1)->f_autos != ici_funcof(o2)->f_autos
        || ici_funcof(o1)->f_args != ici_funcof(o2)->f_args
        || ici_funcof(o1)->f_name != ici_funcof(o2)->f_name;
}

unsigned long func_type::hash(ici_obj_t *o)
{
    return (unsigned long)ici_funcof(o)->f_code * FUNC_PRIME;
}

ici_obj_t * func_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    ici_obj_t           *r;

    ici_error = NULL;
    r = NULL;
    if (k == SS(vars))
    {
        r = ici_funcof(o)->f_autos;
    }
    else if (k == SS(args))
    {
        r = ici_funcof(o)->f_args;
    }
    else if (k == SS(name))
    {
        r = ici_funcof(o)->f_name;
    }
    if (r == NULL && ici_error == NULL)
    {
        r = ici_null;
    }
    return r;
}

void func_type::objname(ici_obj_t *o, char p[ICI_OBJNAMEZ])
{
    ici_str_t   *s;

    s = ici_funcof(o)->f_name;
    if (s->s_nchars > ICI_OBJNAMEZ - 2 - 1)
        sprintf(p, "%.*s...()", ICI_OBJNAMEZ - 6, s->s_chars);
    else
        sprintf(p, "%s()", s->s_chars);
}


/*
 * arg(N-1) .. arg1 arg0 nargs func     => (os) OR
 * arg(N-1) .. arg1 arg0 nargs ptr      => (os) OR
 * arg(N-1) .. arg1 arg0 nargs aggr key => (os) iff ICI_OP_AGGR_KEY_CALL
 *                                => auto-struct  (vs)
 *                      call      => mark pc      (xs)
 *
 * Calling a function pushes a structure for auto variables on the
 * variable stack. It then pushes a mark and a pc starting at the first
 * element of the code array on the execution stack. Any arguments are
 * assigned to the corresponding formal argument names in the auto var
 * structure.
 */
int func_type::call(ici_obj_t *o, ici_obj_t *subject)
{
    func *f;
    ici_struct *d;     /* The local variable structure. */
    ici_obj_t  **ap;   /* Actual parameter. */
    ici_obj_t  **fp;   /* Formal parameter. */
    ici_sslot_t *sl;
    array       *va;
    int         n;

    f = ici_funcof(o);
#ifndef NOPROFILE
    if (UNLIKELY(ici_profile_active))
    {
        ici_profile_call(f);
    }
#endif

    d = ici_structof(f->f_autos->copy());
    if (UNLIKELY(d == NULL))
    {
        goto fail;
    }
    if (UNLIKELY(subject != NULL))
    {
        /*
         * This is a method call, that is, it has a subject object that
         * becomes the scope.
         */
        if (UNLIKELY(!ici_hassuper(subject)))
        {
            char n1[ICI_OBJNAMEZ];
            ici_set_error("attempt to call method on %s", ici_objname(n1, subject));
            goto fail;
        }
        ici_objwsupof(d)->o_super = ici_objwsupof(subject);
        /*
         * Set the special instantiation variables.
         */
        if (UNLIKELY(ici_assign_base(d, SS(this), subject)))
        {
            goto fail;
        }
        if
        (
            UNLIKELY
            (
                ici_objwsupof(f->f_autos)->o_super != NULL
                &&
                ici_assign_base(d, SS(class), ici_objwsupof(f->f_autos)->o_super)
            )
        )
        {
            goto fail;
        }
    }
    n = NARGS(); /* Number of actual args. */
    ap = ARGS();
    if (LIKELY(f->f_args != NULL))
    {
        /*
         * There are explicit formal parameters.
         */
        fp = f->f_args->a_base;
        /*
         * Assign the actuals to the formals.
         */
        while (fp < f->f_args->a_top && n > 0)
        {
            assert(ici_isstring(*fp));
            if (LIKELY(ici_stringof(*fp)->s_struct == d && ici_stringof(*fp)->s_vsver == vsver))
            {
                ici_stringof(*fp)->s_slot->sl_value = *ap;
            }
            else
            {
                if (UNLIKELY(ici_assign(d, *fp, *ap)))
                {
                    goto fail;
                }
            }
            ++fp;
            --ap;
            --n;
        }
    }
    va = NULL;
    if (UNLIKELY(n > 0))
    {
        if
        (
            LIKELY
            (
                (sl = ici_find_raw_slot(d, SS(vargs))) != NULL
                &&
                (va = ici_array_new(n)) != NULL
            )
        )
        {
            /*
             * There are left-over actual parameters and a "vargs"
             * auto to put them in, and everything else looks good.
             */
            while (--n >= 0)
            {
                *va->a_top++ = *ap--;
            }
            sl->sl_value = va;
            va->decref();
        }
    }

    /*
     * we push the current source marker onto the execution stack.
     * That way, after the function returns, it will cause the current
     * source marker to be reset to the correct value.
     */
    ici_xs.a_top[-1] = ici_exec->x_src;

    *ici_xs.a_top++ = &ici_o_mark;
    ici_get_pc(f->f_code, ici_xs.a_top);
    ++ici_xs.a_top;
    *ici_vs.a_top++ = d;
    d->decref();
    ici_os.a_top -= NARGS() + 2;
    return 0;

 fail:
    if (d != NULL)
    {
        d->decref();
    }
    return 1;
}

ici_op_t    ici_o_return{ici_op_return};
ici_op_t    ici_o_call{ICI_OP_CALL};
ici_op_t    ici_o_method_call{ICI_OP_METHOD_CALL};
ici_op_t    ici_o_super_call{ICI_OP_SUPER_CALL};

} // namespace ici

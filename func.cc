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
#include "catcher.h"
#include "buf.h"
#include "mark.h"
#include "null.h"
#include "primes.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

namespace ici
{

func *new_func()
{
    func *f;

    if ((f = ici_talloc(func)) == NULL)
    {
        return NULL;
    }
    memset((char *)f, 0, sizeof (func));
    ICI_OBJ_SET_TFNZ(f, ICI_TC_FUNC, 0, 1, 0);
    ici_rego(f);
    return f;
}

int op_return()
{
    static int   occasionally = 0;
    object     **x;
    object      *f;

    if (UNLIKELY(ici_debug_active))
    {
        debugfunc->idbg_fnresult(os.a_top[-1]);
    }

    x = xs.a_top - 1;
    while
    (
        !ismark(*x)
        &&
        --x >= xs.a_base
        &&
        !(iscatcher(*x) && isnull(catcherof(*x)->c_catcher))
    )
        ;
    if (x < xs.a_base || !ismark(*x))
    {
        return set_error("return not in function");
    }
    xs.a_top = x;

    /*
     * If convenient, record the total nels of the autos that this function
     * ended up with, as a hint for the auto struct allocation on next call.
     * If it isn't convenient, do it occasionally anyway.
     */
    if
    (
        SS(_func_)->s_struct == vs.a_top[-1]
        &&
        SS(_func_)->s_vsver == vsver
        &&
        isfunc(f = SS(_func_)->s_slot->sl_value)
    )
    {
        funcof(f)->f_nautos = structof(vs.a_top[-1])->s_nels;
    }
    else if (--occasionally <= 0)
    {
        occasionally = 10;
        f = ici_fetch(vs.a_top[-1], SS(_func_));
        if (isstruct(vs.a_top[-1]) && isfunc(f))
            funcof(f)->f_nautos = structof(vs.a_top[-1])->s_nels;
    }

    --vs.a_top;
#ifndef NOPROFILE
    if (ici_profile_active)
        ici_profile_return();
#endif
    return 0;
}

size_t func_type::mark(object *o)
{
    auto fn = funcof(o);
    return type::mark(fn)
        + maybe_mark(fn->f_code)
        + maybe_mark(fn->f_args)
        + maybe_mark(fn->f_autos)
        + maybe_mark(fn->f_name);
}

int func_type::cmp(object *o1, object *o2)
{
    return funcof(o1)->f_code != funcof(o2)->f_code
        || funcof(o1)->f_autos != funcof(o2)->f_autos
        || funcof(o1)->f_args != funcof(o2)->f_args
        || funcof(o1)->f_name != funcof(o2)->f_name;
}

unsigned long func_type::hash(object *o)
{
    return (unsigned long)funcof(o)->f_code * FUNC_PRIME;
}

object * func_type::fetch(object *o, object *k)
{
    object           *r;

    error = NULL;
    r = NULL;
    if (k == SS(vars))
    {
        r = funcof(o)->f_autos;
    }
    else if (k == SS(args))
    {
        r = funcof(o)->f_args;
    }
    else if (k == SS(name))
    {
        r = funcof(o)->f_name;
    }
    if (r == NULL && error == NULL)
    {
        r = ici_null;
    }
    return r;
}

void func_type::objname(object *o, char p[objnamez])
{
    str   *s;

    s = funcof(o)->f_name;
    if (s->s_nchars > objnamez - 2 - 1)
        sprintf(p, "%.*s...()", objnamez - 6, s->s_chars);
    else
        sprintf(p, "%s()", s->s_chars);
}


/*
 * arg(N-1) .. arg1 arg0 nargs func     => (os) OR
 * arg(N-1) .. arg1 arg0 nargs ptr      => (os) OR
 * arg(N-1) .. arg1 arg0 nargs aggr key => (os) iff OP_AGGR_KEY_CALL
 *                                => auto-struct  (vs)
 *                      call      => mark pc      (xs)
 *
 * Calling a function pushes a structure for auto variables on the
 * variable stack. It then pushes a mark and a pc starting at the first
 * element of the code array on the execution stack. Any arguments are
 * assigned to the corresponding formal argument names in the auto var
 * structure.
 */
int func_type::call(object *o, object *subject)
{
    func *f;
    ici_struct *d;     /* The local variable structure. */
    object  **ap;   /* Actual parameter. */
    object  **fp;   /* Formal parameter. */
    sslot *sl;
    array       *va;
    int         n;

    f = funcof(o);
#ifndef NOPROFILE
    if (UNLIKELY(ici_profile_active))
    {
        ici_profile_call(f);
    }
#endif

    d = structof(f->f_autos->copy());
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
        if (UNLIKELY(!hassuper(subject)))
        {
            char n1[objnamez];
            set_error("attempt to call method on %s", ici_objname(n1, subject));
            goto fail;
        }
        objwsupof(d)->o_super = objwsupof(subject);
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
                objwsupof(f->f_autos)->o_super != NULL
                &&
                ici_assign_base(d, SS(class), objwsupof(f->f_autos)->o_super)
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
            assert(isstring(*fp));
            if (LIKELY(stringof(*fp)->s_struct == d && stringof(*fp)->s_vsver == vsver))
            {
                stringof(*fp)->s_slot->sl_value = *ap;
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
                (sl = find_raw_slot(d, SS(vargs))) != NULL
                &&
                (va = new_array(n)) != NULL
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
    xs.a_top[-1] = ex->x_src;

    *xs.a_top++ = &o_mark;
    get_pc(f->f_code, xs.a_top);
    ++xs.a_top;
    *vs.a_top++ = d;
    d->decref();
    os.a_top -= NARGS() + 2;
    return 0;

 fail:
    if (d != NULL)
    {
        d->decref();
    }
    return 1;
}

op    o_return{op_return};
op    o_call{OP_CALL};
op    o_method_call{OP_METHOD_CALL};
op    o_super_call{OP_SUPER_CALL};

} // namespace ici

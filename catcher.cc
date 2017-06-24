#define ICI_CORE
/*
 * catch.c - implementation of the ICI catch type. This is an internal
 * type used as a marker on the execution stack. These objects should
 * never escape to user visibility. See also catch.h.
 */
#include "exec.h"
#include "pc.h"
#include "catcher.h"
#include "op.h"
#include "func.h"

namespace ici
{

/*
 * Unwind the execution stack until a catcher is found.  Then unwind
 * the scope and operand stacks to the matching depth (but only if it is).
 * Returns the catcher, or NULL if there wasn't one.
 */
catcher *ici_unwind()
{
    object   **p;
    catcher *c;

    for (p = ici_xs.a_top - 1; p >= ici_xs.a_base; --p)
    {
        if (iscatcher(*p))
        {
            c = catcherof(*p);
            ici_xs.a_top = p;
            ici_os.a_top = &ici_os.a_base[c->c_odepth];
            ici_vs.a_top = &ici_vs.a_base[c->c_vdepth];
            return c;
        }
    }
    assert(0);
    return NULL;
}

/*
 * Return a new catcher object with the given catcher object and
 * corresponding to the operand and variable stack depths given.
 * The catcher, o, may be NULL.
 *
 * Note: catch's are special. Unlike most types this ici_new_catch function
 * returns its object with a ref count of 0 not 1.
 */
catcher *ici_new_catcher(object *o, int odepth, int vdepth, int flags)
{
     catcher *c;

    if ((c = ici_talloc(catcher)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(c, ICI_TC_CATCHER, flags, 0, 0);
    c->c_catcher = o;
    c->c_odepth = odepth;
    c->c_vdepth = vdepth;
    ici_rego(c);
    return c;
}

/*
 * runner handler       => catcher (os)
 *                      => catcher pc (xs)
 *                      => catcher (vs)
 */
int ici_op_onerror()
{
    if ((ici_xs.a_top[-1] = ici_new_catcher(ici_os.a_top[-1], ici_os.a_top - ici_os.a_base - 2, ici_vs.a_top - ici_vs.a_base, 0)) == NULL)
        return 1;
    ici_get_pc(arrayof(ici_os.a_top[-2]), ici_xs.a_top);
    ++ici_xs.a_top;
    ici_os.a_top -= 2;
    return 0;
}

size_t catcher_type::mark(object *o)
{
    auto c = catcherof(o);
    return setmark(c) + maybe_mark(c->c_catcher);
}

void catcher_type::free(object *o)
{
    assert(!o->flagged(CF_EVAL_BASE));
    ici_tfree(o, catcher);
}

ici_op_t    ici_o_onerror{ici_op_onerror};

} // namespace ici

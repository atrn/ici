#define ICI_CORE
/*
 * catch.c - implementation of the ICI catch type. This is an internal
 * type used as a marker on the execution stack. These objects should
 * never escape to user visibility. See also catch.h.
 */
#include "catcher.h"
#include "exec.h"
#include "func.h"
#include "op.h"
#include "pc.h"

namespace ici
{

/*
 * Return a new catcher object with the given catcher object and
 * corresponding to the operand and variable stack depths given.
 * The catcher, o, may be nullptr.
 *
 * Note: catch's are special. Unlike most types this new_catcher function
 * returns its object with a ref count of 0 not 1.
 */
catcher *new_catcher(object *o, int odepth, int vdepth, int flags)
{
    catcher *c;

    if ((c = ici_talloc(catcher)) == nullptr)
    {
        return nullptr;
    }
    set_tfnz(c, TC_CATCHER, flags, 0, 0);
    c->c_catcher = o;
    c->c_odepth = odepth;
    c->c_vdepth = vdepth;
    rego(c);
    return c;
}

/*
 * runner handler       => catcher (os)
 *                      => catcher pc (xs)
 *                      => catcher (vs)
 */
int op_onerror()
{
    if ((xs.a_top[-1] = new_catcher(os.a_top[-1], os.a_top - os.a_base - 2, vs.a_top - vs.a_base, 0)) == nullptr)
    {
        return 1;
    }
    set_pc(arrayof(os.a_top[-2]), xs.a_top);
    ++xs.a_top;
    os.a_top -= 2;
    return 0;
}

size_t catcher_type::mark(object *o)
{
    auto c = catcherof(o);
    return type::mark(c) + mark_optional(c->c_catcher);
}

void catcher_type::free(object *o)
{
    assert(!o->hasflag(CF_EVAL_BASE));
    ici_tfree(o, catcher);
}

op o_onerror{op_onerror};

} // namespace ici

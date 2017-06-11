#define ICI_CORE
/*
 * catch.c - implementation of the ICI catch type. This is an internal
 * type used as a marker on the execution stack. These objects should
 * never escape to user visibility. See also catch.h.
 */
#include "exec.h"
#include "pc.h"
#include "catch.h"
#include "op.h"
#include "func.h"

namespace ici
{

/*
 * Unwind the execution stack until a catcher is found.  Then unwind
 * the scope and operand stacks to the matching depth (but only if it is).
 * Returns the catcher, or NULL if there wasn't one.
 */
ici_catch_t *
ici_unwind()
{
    ici_obj_t   **p;
    ici_catch_t *c;

    for (p = ici_xs.a_top - 1; p >= ici_xs.a_base; --p)
    {
        if (ici_iscatch(*p))
        {
            c = ici_catchof(*p);
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
 * Return a new catch object with the given catcher object and
 * corresponding to the operand and variable stack depths given.
 * The catcher, o, may be NULL.
 *
 * Note: catch's are special. Unlike most types this ici_new_catch function
 * returns its object with a ref count of 0 not 1.
 */
ici_catch_t *
ici_new_catch(ici_obj_t *o, int odepth, int vdepth, int flags)
{
     ici_catch_t    *c;

    if ((c = ici_talloc(ici_catch_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(c, ICI_TC_CATCH, flags, 0, 0);
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
int
ici_op_onerror()
{
    if ((ici_xs.a_top[-1] = ici_new_catch(ici_os.a_top[-1], ici_os.a_top - ici_os.a_base - 2, ici_vs.a_top - ici_vs.a_base, 0)) == NULL)
        return 1;
    ici_get_pc(ici_arrayof(ici_os.a_top[-2]), ici_xs.a_top);
    ++ici_xs.a_top;
    ici_os.a_top -= 2;
    return 0;
}

class catch_type : public type
{
public:
    catch_type() : type("catch") {}

    unsigned long mark(ici_obj_t *o) override
    {
        unsigned long       mem;

        o->o_flags |= ICI_O_MARK;
        mem = sizeof(ici_catch_t);
        if (ici_catchof(o)->c_catcher != NULL)
            mem += ici_mark(ici_catchof(o)->c_catcher);
        return mem;
    }

    void free(ici_obj_t *o) override
    {
        assert((o->o_flags & CF_EVAL_BASE) == 0);
        ici_tfree(o, ici_catch_t);
    }

};

ici_op_t    ici_o_onerror       = {ici_op_onerror};

} // namespace ici

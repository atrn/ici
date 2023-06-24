#define ICI_CORE
#include "forall.h"
#include "exec.h"
#include "null.h"

namespace ici
{

/*
 * va vk ka kk aggr code        => (os)
 *                              => forall (xs)
 */
int op_forall()
{
    forall *fa;

    if (os.a_top[-2] == null)
    {
        os.a_top -= 6;
        --xs.a_top;
        return 0;
    }
    if ((fa = ici_talloc(forall)) == nullptr)
    {
        return 1;
    }
    set_tfnz(fa, TC_FORALL, 0, 0, 0);
    fa->fa_index = size_t(-1);
    fa->fa_code = *--os.a_top;
    fa->fa_aggr = *--os.a_top;
    fa->fa_kkey = *--os.a_top;
    fa->fa_kaggr = *--os.a_top;
    fa->fa_vkey = *--os.a_top;
    fa->fa_vaggr = *--os.a_top;
    xs.a_top[-1] = fa;
    rego(fa);
    return 0;
}

/*
 * forall => forall pc (xs)
 *  OR
 * forall => (xs)
 */
int exec_forall()
{
    forall *fa;
    type   *t;

    fa = forallof(xs.a_top[-1]);
    t = fa->fa_aggr->icitype();
    switch (t->forall(fa))
    {
    case 0:
        set_pc(arrayof(fa->fa_code), xs.a_top);
        ++xs.a_top;
        return 0;

    case -1:
        --xs.a_top;
        return 0;

    default:
        return 1;
    }
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t forall_type::mark(object *o)
{
    auto fa = forallof(o);
    auto mem = type::mark(o);
    mem += mark_optional(fa->fa_aggr);
    mem += mark_optional(fa->fa_code);
    mem += mark_optional(fa->fa_vaggr);
    mem += mark_optional(fa->fa_vkey);
    mem += mark_optional(fa->fa_kaggr);
    mem += mark_optional(fa->fa_kkey);
    return mem;
}

} // namespace ici

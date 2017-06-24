#define ICI_CORE
#include "forall.h"
#include "exec.h"
#include "pc.h"
#include "struct.h"
#include "set.h"
#include "forall.h"
#include "str.h"
#include "buf.h"
#include "null.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
/*
 * va vk ka kk aggr code        => (os)
 *                              => forall (xs)
 */
int op_forall()
{
    forall *fa;

    if (os.a_top[-2] == ici_null)
    {
        os.a_top -= 6;
        --xs.a_top;
        return 0;
    }
    if ((fa = ici_talloc(forall)) == NULL)
    {
        return 1;
    }
    ICI_OBJ_SET_TFNZ(fa, ICI_TC_FORALL, 0, 0, 0);
    fa->fa_index = size_t(-1);
    fa->fa_code = *--os.a_top;
    fa->fa_aggr = *--os.a_top;
    fa->fa_kkey = *--os.a_top;
    fa->fa_kaggr = *--os.a_top;
    fa->fa_vkey = *--os.a_top;
    fa->fa_vaggr = *--os.a_top;
    xs.a_top[-1] = fa;
    ici_rego(fa);
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
    type *t;

    fa = forallof(xs.a_top[-1]);
    t = fa->fa_aggr->otype();
    if (!t->can_forall())
    {
        char n[objnamez];
        return set_error("attempt to forall over %s", ici_objname(n, fa->fa_aggr));
    }
    switch (t->forall(fa))
    {
    case 0:
        ici_get_pc(arrayof(fa->fa_code), xs.a_top);
        ++xs.a_top;
        return 0;

    case -1:
        --xs.a_top;
        return 0;

    default:
        return 1;
    }
}

size_t forall_type::mark(object *o)
{
    auto fa = forallof(o);
    auto mem = type::mark(o);
    mem += maybe_mark(fa->fa_aggr);
    mem += maybe_mark(fa->fa_code);
    mem += maybe_mark(fa->fa_vaggr);
    mem += maybe_mark(fa->fa_vkey);
    mem += maybe_mark(fa->fa_kaggr);
    mem += maybe_mark(fa->fa_kkey);
    return mem;
}

} // namespace ici

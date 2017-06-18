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
int
ici_op_forall()
{
    ici_forall_t   *fa;

    if (ici_os.a_top[-2] == ici_null)
    {
        ici_os.a_top -= 6;
        --ici_xs.a_top;
        return 0;
    }
    if ((fa = ici_talloc(ici_forall_t)) == NULL)
    {
        return 1;
    }
    ICI_OBJ_SET_TFNZ(fa, ICI_TC_FORALL, 0, 0, 0);
    ici_rego(fa);
    fa->fa_index = -1;
    fa->fa_code = *--ici_os.a_top;
    fa->fa_aggr = *--ici_os.a_top;
    fa->fa_kkey = *--ici_os.a_top;
    fa->fa_kaggr = *--ici_os.a_top;
    fa->fa_vkey = *--ici_os.a_top;
    fa->fa_vaggr = *--ici_os.a_top;
    ici_xs.a_top[-1] = fa;
    return 0;
}

/*
 * forall => forall pc (xs)
 *  OR
 * forall => (xs)
 */
int
ici_exec_forall()
{
    forall *fa;
    type *t;

    fa = forallof(ici_xs.a_top[-1]);
    t = ici_typeof(fa->fa_aggr);
    if (!t->can_forall())
    {
        char n[ICI_OBJNAMEZ];
        return ici_set_error("attempt to forall over %s", ici_objname(n, fa->fa_aggr));
    }
    switch (t->forall(fa))
    {
    case 0:
        ici_get_pc(arrayof(fa->fa_code), ici_xs.a_top);
        ++ici_xs.a_top;
        return 0;

    case -1:
        --ici_xs.a_top;
        return 0;

    default:
        return 1;
    }
}

inline unsigned long maybe_mark(ici_obj_t *o) {
    return o == nullptr ? 0 : o->mark();
}

size_t forall_type::mark(ici_obj_t *o)
{
    o->setmark();
    auto fa = forallof(o);
    auto mem = typesize();
    mem += maybe_mark(fa->fa_aggr);
    mem += maybe_mark(fa->fa_code);
    mem += maybe_mark(fa->fa_vaggr);
    mem += maybe_mark(fa->fa_vkey);
    mem += maybe_mark(fa->fa_kaggr);
    mem += maybe_mark(fa->fa_kkey);
    return mem;
}

} // namespace ici

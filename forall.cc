#define ICI_CORE
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
    ici_forall_t        *fa;
    type_t              *t;

    fa = forallof(ici_xs.a_top[-1]);
    t = ici_typeof(fa->fa_aggr);
    if (!t->can_forall())
    {
        char n[ICI_OBJNAMEZ+1];
        return ici_set_error("attempt to forall over %s", ici_objname(n, fa->fa_aggr));
    }
    switch (t->forall(fa))
    {
    case 0:
        ici_get_pc(ici_arrayof(fa->fa_code), ici_xs.a_top);
        ++ici_xs.a_top;
        return 0;

    case -1:
        --ici_xs.a_top;
        return 0;

    default:
        return 1;
    }
}

unsigned long forall_type::mark(ici_obj_t *o)
{
    int        i;
    unsigned long       mem;

    o->o_flags |= ICI_O_MARK;
    mem = sizeof(ici_forall_t);
    for (i = 0; i < (int)nels(forallof(o)->fa_objs); ++i)
    {
        if (forallof(o)->fa_objs[i] != NULL)
        {
            mem += ici_mark(forallof(o)->fa_objs[i]);
        }
    }
    return mem;
}

} // namespace ici

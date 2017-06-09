#define ICI_CORE
#include "exec.h"
#include "pc.h"
#include "struct.h"
#include "set.h"
#include "forall.h"
#include "str.h"
#include "buf.h"
#include "null.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_forall(ici_obj_t *o)
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
    ici_xs.a_top[-1] = ici_objof(fa);
    return 0;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_forall(ici_obj_t *o)
{
    ici_tfree(o, ici_forall_t);
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
    int                 (*step)(ici_obj_t *);

    fa = forallof(ici_xs.a_top[-1]);
    step = ici_typeof(fa->fa_aggr)->t_forall_step;
    if (step == NULL)
    {
        char n[ICI_OBJNAMEZ+1];
        return ici_set_error("attempt to forall over %s", ici_objname(n, fa->fa_aggr));
    }
    switch (step(ici_objof(fa)))
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

ici_type_t  ici_forall_type =
{
    mark_forall,
    free_forall,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "forall"
};

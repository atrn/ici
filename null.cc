#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_null(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof(ici_null_t);
}

type_t  null_type =
{
    mark_null,
    NULL,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "NULL"
};

ici_null_t  ici_o_null; //  = {ICI_TC_NULL, ICI_O_ATOM, 1, 0};

} // namespace ici

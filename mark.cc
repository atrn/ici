#define ICI_CORE
#include "fwd.h"
#include "mark.h"

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof(ici_mark_t);
}

ici_type_t  ici_mark_type =
{
    mark_mark,
    NULL,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "mark"
};

ici_mark_t  ici_o_mark;

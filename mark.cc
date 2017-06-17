#define ICI_CORE
#include "fwd.h"
#include "mark.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long mark_type::mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (ici_mark_t);
}

void mark_type::free(ici_obj_t *)
{
}

ici_mark_t  ici_o_mark;

} // namespace ici

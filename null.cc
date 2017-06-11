#define ICI_CORE
#include "fwd.h"
#include "null.h"
#include "types.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long null_type::mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (ici_null_t);
}

void null_type::free(ici_obj_t *o)
{
}

ici_null_t  ici_o_null;

} // namespace ici

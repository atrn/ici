#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
class null_type : public type
{
public:
    null_type() : type("null") {}

    unsigned long mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        return sizeof (ici_null_t);
    }

    void free(ici_obj_t *o) override
    {
    }
};

ici_null_t  ici_o_null;

} // namespace ici

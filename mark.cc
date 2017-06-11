#define ICI_CORE
#include "fwd.h"
#include "mark.h"

namespace ici
{

class mark_type : public type
{
public:
    mark_type() : type("mark") {}

    /*
     * Mark this and referenced unmarked objects, return memory costs.
     * See comments on t_mark() in object.h.
     */
    unsigned long mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        return sizeof (ici_mark_t);
    }

    void free(ici_obj_t *) override
    {
    }
};

ici_mark_t  ici_o_mark;

} // namespace ici

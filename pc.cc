#define ICI_CORE
#include "exec.h"
#include "pc.h"

namespace ici
{

/*
 * Return a new pc object.
 *
 * NOTE: pc's come back ready-decref'ed.
 */
ici_pc_t *
ici_new_pc()
{
    ici_pc_t            *pc;

    if ((pc = ici_pcof(ici_talloc(ici_pc_t))) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(pc, ICI_TC_PC, 0, 0, 0);
    pc->pc_code = NULL;
    ici_rego(pc);
    return pc;
}

class pc_type : public type
{
public:
    pc_type() : type("pc") {}

    /*
     * Mark this and referenced unmarked objects, return memory costs.
     * See comments on t_mark() in object.h.
     */
    unsigned long
    mark(ici_obj_t *o) override
    {
        o->o_flags |= ICI_O_MARK;
        if (ici_pcof(o)->pc_code != NULL)
            return sizeof(ici_pc_t) + ici_mark(ici_pcof(o)->pc_code);
        return sizeof(ici_pc_t);
    }

    /*
     * Free this object and associated memory (but not other objects).
     * See the comments on t_free() in object.h.
     */
    void free(ici_obj_t *o) override
    {
        ici_tfree(o, ici_pc_t);
    }

};

} // namespace ici

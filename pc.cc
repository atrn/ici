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

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long pc_type::mark(ici_obj_t *o)
{
    o->setmark();
    auto mem = typesize();
    if (ici_pcof(o)->pc_code != NULL)
        mem += ici_mark(ici_pcof(o)->pc_code);
    return mem;
}

} // namespace ici

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
pc *ici_new_pc()
{
    pc *p;

    if ((p = ici_pcof(ici_talloc(pc))) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(p, ICI_TC_PC, 0, 0, 0);
    p->pc_code = NULL;
    ici_rego(p);
    return p;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t pc_type::mark(object *o)
{
    auto p = ici_pcof(o);
    auto mem = typesize();
    p->setmark();
    if (p->pc_code != NULL)
        mem += ici_mark(p->pc_code);
    return mem;
}

} // namespace ici

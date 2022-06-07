#define ICI_CORE
#include "pc.h"
#include "exec.h"

namespace ici
{

/*
 * Return a new pc object.
 *
 * NOTE: pc's come back ready-decref'ed.
 */
pc *new_pc()
{
    pc *p;

    if ((p = ici_talloc(pc)) == nullptr)
    {
        return nullptr;
    }
    set_tfnz(p, TC_PC, 0, 0, 0);
    p->pc_code = nullptr;
    rego(p);
    return p;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t pc_type::mark(object *o)
{
    auto p = pcof(o);
    p->setmark();
    return objectsize() + mark_optional(p->pc_code);
}

} // namespace ici

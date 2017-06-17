#define ICI_CORE
#include "exec.h"
#include "src.h"
#include "str.h"

namespace ici
{

ici_src_t *
ici_src_new(int lineno, ici_str_t *filename)
{
    ici_src_t  *s;

    if ((s = ici_talloc(ici_src_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, ICI_TC_SRC, 0, 1, 0);
    s->s_lineno = lineno;
    s->s_filename = filename;
    ici_rego(s);
    return s;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long src_type::mark(ici_obj_t *o)
{
    o->setmark();
    auto mem = typesize();
    if (ici_srcof(o)->s_filename != NULL)
        mem += ici_mark(ici_srcof(o)->s_filename);
    return mem;
}

} // namespace ici

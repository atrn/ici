#define ICI_CORE
#include "exec.h"
#include "src.h"
#include "str.h"

namespace ici
{

src *ici_src_new(int lineno, ici_str_t *filename)
{
    src *s;

    if ((s = ici_talloc(src)) == NULL)
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
size_t src_type::mark(object *o)
{
    o->setmark();
    auto mem = typesize();
    if (srcof(o)->s_filename != NULL)
        mem += ici_mark(srcof(o)->s_filename);
    return mem;
}

} // namespace ici

#define ICI_CORE
#include "exec.h"
#include "src.h"
#include "str.h"

namespace ici
{

src *new_src(int lineno, str *filename)
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

size_t src_type::mark(object *o)
{
    o->setmark();
    auto mem = typesize();
    if (srcof(o)->s_filename)
        mem += srcof(o)->s_filename->mark();
    return mem;
}

} // namespace ici

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
    set_tfnz(s, TC_SRC, 0, 1, 0);
    s->s_lineno = lineno;
    s->s_filename = filename;
    rego(s);
    return s;
}

size_t src_type::mark(object *o)
{
    auto s = srcof(o);
    auto mem = size();
    s->setmark();
    if (s->s_filename)
        mem += ici_mark(s->s_filename);
    return mem;
}

} // namespace ici

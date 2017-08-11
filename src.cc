#define ICI_CORE
#include "exec.h"
#include "src.h"
#include "str.h"
#include "archiver.h"

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

int src_type::save(archiver *ar, object *o) {
    return ar->write(int32_t(srcof(o)->s_lineno)) || ar->save(srcof(o)->s_filename);
}

object *src_type::restore(archiver *ar) {
    int32_t line;
    object *result;
    object *filename;

    if (ar->read(line)) {
        return NULL;
    }
    if ((filename = ar->restore()) == NULL) {
        return NULL;
    }
    if (!isstring(filename)) {
        set_error("unexpected filename type (%s)", filename->type_name());
        filename->decref();
        return NULL;
    }
    if ((result = new_src(line, stringof(filename))) == NULL) {
        filename->decref();
        return NULL;
    }
    filename->decref();
    return result;
}

} // namespace ici

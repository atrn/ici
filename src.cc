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

    if ((s = ici_talloc(src)) == nullptr)
        return nullptr;
    set_tfnz(s, TC_SRC, 0, 1, 0);
    s->s_lineno = lineno;
    s->s_filename = filename;
    rego(s);
    return s;
}

size_t src_type::mark(object *o)
{
    auto s = srcof(o);
    s->setmark();
    return objectsize() + mark_optional(s->s_filename);
}

int src_type::save(archiver *ar, object *o) {
    const int32_t lineno = srcof(o)->s_lineno;
    if (ar->write(lineno)) {
        return 1;
    }
    if (ar->save(srcof(o)->s_filename)) {
        return 1;
    }
    return 0;
}

object *src_type::restore(archiver *ar) {
    int32_t line;
    object *result;
    object *filename;

    if (ar->read(&line)) {
        return nullptr;
    }
    if ((filename = ar->restore()) == nullptr) {
        return nullptr;
    }
    if (!isstring(filename)) {
        set_error("unexpected filename type (%s)", filename->type_name());
        filename->decref();
        return nullptr;
    }
    if ((result = new_src(line, stringof(filename))) == nullptr) {
        filename->decref();
        return nullptr;
    }
    filename->decref();
    return result;
}

} // namespace ici

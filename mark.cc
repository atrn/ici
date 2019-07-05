#define ICI_CORE
#include "fwd.h"
#include "mark.h"

namespace ici
{

mark::mark() : object(TC_MARK) {}

mark o_mark;

void mark_type::free(object *) {}

int mark_type::save(archiver *, object *) {
    return 0;
}

object *mark_type::restore(archiver *) {
    return copyof(&o_mark);
}

} // namespace ici

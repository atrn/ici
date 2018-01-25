#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici {

void null_type::free(object *) {
    // null is static and is never freed
}

int null_type::save(archiver *, object *) {
    return 0;
}

object *null_type::restore(archiver *) {
    return ici_copy(null);
}

struct null o_null;

struct null *null = &o_null;

} // namespace ici

#define ICI_CORE
#include "fwd.h"
#include "mark.h"

namespace ici
{

mark::mark() : object(TC_MARK) {}

void mark_type::free(object *) {}

mark o_mark;

} // namespace ici

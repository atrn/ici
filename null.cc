#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici
{

void null_type::free(object *o)
{
}

ici_null_t  ici_o_null;

} // namespace ici

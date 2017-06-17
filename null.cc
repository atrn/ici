#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici
{

void null_type::free(ici_obj_t *o)
{
}

ici_null_t  ici_o_null;

} // namespace ici

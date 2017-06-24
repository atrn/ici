#define ICI_CORE
#include "fwd.h"
#include "null.h"

namespace ici
{

void null_type::free(object *)
{
}

null o_null;

} // namespace ici

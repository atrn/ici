// -*- mode:c++ -*-

#ifndef ICI_NULL_H
#define ICI_NULL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_null_t : ici_obj
{
    ici_null_t() : ici_obj{ICI_TC_NULL, ICI_O_ATOM, 1, 0} {}
};
#define ici_nullof(o)       (static_cast<ici_null_t *>(o))
inline bool ici_isnull(ici_obj_t *o) { return o == ici_objof(ici_null); }
// #define ici_isnull(o)       (ici_objof(o) == ici_objof(ici_null))

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_NULL_H */

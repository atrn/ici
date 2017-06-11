// -*- mode:c++ -*-

#ifndef ICI_PC_H
#define ICI_PC_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_pc : object
{
    ici_array_t *pc_code;
    ici_obj_t   **pc_next;
};

inline ici_pc_t * ici_pcof(ici_obj_t *o) { return static_cast<ici_pc_t *>(o); }
inline bool ici_ispc(ici_obj_t *o) { return o->isa(ICI_TC_PC); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif  /* ICI_PC_H */

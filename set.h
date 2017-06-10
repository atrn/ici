// -*- mode:c++ -*-

#ifndef ICI_SETS_H
#define ICI_SETS_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_set : ici_obj
{
    int         s_nels;         /* How many slots used. */
    int         s_nslots;       /* How many slots allocated. */
    ici_obj_t   **s_slots;
};

inline ici_set_t *ici_setof(ici_obj_t *o) { return static_cast<ici_set_t *>(o); }
inline bool ici_isset(ici_obj_t *o) { return o->isa(ICI_TC_SET); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_SETS_H */

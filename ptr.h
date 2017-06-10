// -*- mode:c++ -*-

#ifndef ICI_PTR_H
#define ICI_PTR_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_ptr : ici_obj
{
    ici_obj_t   *p_aggr;        /* The aggregate which contains the object. */
    ici_obj_t   *p_key;         /* The key which references it. */
};

inline ici_ptr_t *ici_ptrof(ici_obj_t *o) { return static_cast<ici_ptr_t *>(o); }
inline bool ici_isptr(ici_obj_t *o) { return o->isa(ICI_TC_PTR); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif  /* ICI_PTR_H */

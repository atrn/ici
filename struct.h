// -*- mode:c++ -*-

#ifndef ICI_STRUCT_H
#define ICI_STRUCT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct sslot
{
    ici_obj_t   *sl_key;
    ici_obj_t   *sl_value;
};

struct ici_struct : objwsup
{
    int         s_nels;         /* How many slots used. */
    int         s_nslots;       /* How many slots allocated. */
    ici_sslot_t *s_slots;
};

inline ici_struct_t *ici_structof(ici_obj_t *o) { return static_cast<ici_struct_t *>(o); }
inline bool ici_isstruct(ici_obj_t *o)          { return o->isa(ICI_TC_STRUCT); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_STRUCT_H */

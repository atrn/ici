// -*- mode:c++ -*-

#ifndef ICI_FORALL_H
#define ICI_FORALL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_forall : object
{
    int         fa_index;
    ici_obj_t   *fa_objs[6];
};
#define fa_aggr         fa_objs[0]
#define fa_code         fa_objs[1]
#define fa_vaggr        fa_objs[2]
#define fa_vkey         fa_objs[3]
#define fa_kaggr        fa_objs[4]
#define fa_kkey         fa_objs[5]

inline ici_forall_t *forallof(ici_obj_t *o) { return static_cast<ici_forall_t *>(o); }
inline bool isforall(ici_obj_t *o) { return o->isa(ICI_TC_FORALL); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_FORALL_H */

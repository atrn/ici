// -*- mode:c++ -*-

#ifndef ICI_FORALL_H
#define ICI_FORALL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_forall : ici_obj
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

#define forallof(o)     ((ici_forall_t *)o)
#define isforall(o)     ((o)->o_tcode == ICI_TC_FORALL)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_FORALL_H */

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
struct ici_pc : ici_obj
{
    ici_array_t *pc_code;
    ici_obj_t   **pc_next;
};
#define ici_pcof(o)         (static_cast<ici_pc_t *>(o))
#define ici_ispc(o)         ((o)->o_tcode == ICI_TC_PC)
/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif  /* ICI_PC_H */

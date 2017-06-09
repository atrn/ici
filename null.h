// -*- mode:c++ -*-

#ifndef ICI_NULL_H
#define ICI_NULL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_null_t : ici_obj
{
    ici_null_t() : ici_obj{ICI_TC_NULL, ICI_O_ATOM, 1, 0} {}
};
#define ici_nullof(o)       (static_cast<ici_null_t *>(o))
#define ici_isnull(o)       ((o) == ici_null)

/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_NULL_H */

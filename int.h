#ifndef ICI_INT_H
#define ICI_INT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 * The C struct that is the ICI int object.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_int
{
    ici_obj_t   o_head;
    long        i_value;
};
#define ici_intof(o)        ((ici_int_t *)(o))
#define ici_isint(o)        ((o)->o_tcode == ICI_TC_INT)
/*
 * End of ici.h export. --ici.h-end--
 */

#define ICI_SMALL_INT_COUNT (1024)
#define ICI_SMALL_INT_MASK  (0x3FF)
extern ici_int_t            *ici_small_ints[ICI_SMALL_INT_COUNT];

#endif /* ICI_INT_H */

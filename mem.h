#ifndef ICI_MEM_H
#define ICI_MEM_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_mem : ici_obj
{
    void                *m_base;
    size_t              m_length;       /* In m_accessz units. */
    int                 m_accessz;      /* Read/write size. */
    void                (*m_free)(void *);
};

#define ici_memof(o)        ((ici_mem_t *)o)
#define ici_ismem(o)        (ici_objof(o)->o_tcode == ICI_TC_MEM)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_MEM_H */

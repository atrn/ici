// -*- mode:c++ -*-

#ifndef ICI_MEM_H
#define ICI_MEM_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_mem : object
{
    void                *m_base;
    size_t              m_length;       /* In m_accessz units. */
    int                 m_accessz;      /* Read/write size. */
    void                (*m_free)(void *);
};

inline ici_mem_t *ici_memof(ici_obj_t *o) { return static_cast<ici_mem_t *>(o); }
inline bool ici_ismem(ici_obj_t *o) { return o->isa(ICI_TC_MEM); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_MEM_H */

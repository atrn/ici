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
struct mem : object
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

class mem_type : public type
{
public:
    mem_type() : type("mem", sizeof (struct mem)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

} // namespace ici

#endif /* ICI_MEM_H */

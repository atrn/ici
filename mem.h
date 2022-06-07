// -*- mode:c++ -*-

#ifndef ICI_MEM_H
#define ICI_MEM_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct mem : object
{
    void  *m_base;
    size_t m_length;  /* In m_accessz units. */
    int    m_accessz; /* Read/write size in bytes - 1, 2, 4 or 8. */
    void (*m_free)(void *);
};

inline mem *memof(object *o)
{
    return o->as<mem>();
}
inline bool ismem(object *o)
{
    return o->hastype(TC_MEM);
}

class mem_type : public type
{
  public:
    mem_type()
        : type("mem", sizeof(struct mem))
    {
    }
    void          free(object *o) override;
    int           cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    int           assign(object *o, object *k, object *v) override;
    object       *fetch(object *o, object *k) override;
    int           save(archiver *, object *) override;
    object       *restore(archiver *) override;
    int64_t       len(object *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_MEM_H */

// -*- mode:c++ -*-

#ifndef ICI_SETS_H
#define ICI_SETS_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct set : object
{
    size_t   s_nels;   /* How many slots used. */
    size_t   s_nslots; /* How many slots allocated. */
    object **s_slots;
};

int unassign(set *, object *);

inline set *setof(object *o)
{
    return o->as<set>();
}
inline bool isset(object *o)
{
    return o->hastype(TC_SET);
}

class set_type : public type
{
public:
    set_type() : type("set", sizeof(struct set))
    {
    }

    size_t        mark(object *o) override;
    void          free(object *o) override;
    int           cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    object       *copy(object *o) override;
    int           assign(object *o, object *k, object *v) override;
    object       *fetch(object *o, object *k) override;
    int           forall(object *o) override;
    int           save(archiver *, object *) override;
    object       *restore(archiver *) override;
    int64_t       len(object *) override;
    int           nkeys(object *) override;
    int           keys(object *, array *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_SETS_H */

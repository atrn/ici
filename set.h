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
    int      s_nels;            /* How many slots used. */
    int      s_nslots;          /* How many slots allocated. */
    object **s_slots;
};

inline set *setof(object *o) { return static_cast<set *>(o); }
inline bool isset(object *o) { return o->isa(ICI_TC_SET); }

/*
 * End of ici.h export. --ici.h-end--
 */

class set_type : public type
{
public:
    set_type() : type("set", sizeof (struct set), type::has_forall) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    int cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    object *copy(object *o) override;
    int assign(object *o, object *k, object *v) override;
    object *fetch(object *o, object *k) override;
    int forall(object *o) override;
};

} // namespace ici

#endif /* ICI_SETS_H */

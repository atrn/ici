// -*- mode:c++ -*-

#ifndef ICI_MAP_H
#define ICI_MAP_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct slot
{
    object   *sl_key;
    object   *sl_value;
};

struct map : objwsup
{
    size_t      s_nels;         /* How many slots used. */
    size_t      s_nslots;       /* How many slots allocated. */
    slot        *s_slots;
};

inline map *mapof(object *o) { return o->as<map>(); }
inline bool ismap(object *o) { return o->isa(TC_MAP); }

/*
 * End of ici.h export. --ici.h-end--
 */

class map_type : public type
{
public:
    map_type() : type("map", sizeof (map)) {}

    size_t  mark(object *o) override;
    void free(object *o) override;
    unsigned long hash(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *copy(object *o) override;
    int assign_super(object *o, object *k, object *v, map *b) override;
    int assign(object *o, object *k, object *v) override;
    int assign_base(object *o, object *k, object *v) override;
    int forall(object *o) override;
    object *fetch(object *o, object *k) override;
    object *fetch_base(object *o, object *k) override;
    int fetch_super(object *o, object *k, object **pv, map *b) override;
};

} // namespace ici

#endif /* ICI_MAP_H */

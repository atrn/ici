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
    int         s_nels;         /* How many slots used. */
    int         s_nslots;       /* How many slots allocated. */
    ici_obj_t   **s_slots;
};

inline ici_set_t *ici_setof(ici_obj_t *o) { return static_cast<ici_set_t *>(o); }
inline bool ici_isset(ici_obj_t *o) { return o->isa(ICI_TC_SET); }

/*
 * End of ici.h export. --ici.h-end--
 */

class set_type : public type
{
public:
    set_type() : type("set", sizeof (struct set), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 forall(ici_obj_t *o) override;
};

} // namespace ici

#endif /* ICI_SETS_H */

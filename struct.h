// -*- mode:c++ -*-

#ifndef ICI_STRUCT_H
#define ICI_STRUCT_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct sslot
{
    object   *sl_key;
    object   *sl_value;
};

struct ici_struct : objwsup
{
    size_t      s_nels;         /* How many slots used. */
    size_t      s_nslots;       /* How many slots allocated. */
    sslot       *s_slots;
};

inline ici_struct *ici_structof(object *o) { return static_cast<ici_struct *>(o); }

inline bool ici_isstruct(object *o) { return o->isa(ICI_TC_STRUCT); }

/*
 * End of ici.h export. --ici.h-end--
 */

class struct_type : public type
{
public:
    struct_type() : type("struct", sizeof (ici_struct), type::has_forall) {}

    size_t  mark(object *o) override;
    void free(object *o) override;
    unsigned long hash(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *copy(object *o) override;
    int assign_super(object *o, object *k, object *v, ici_struct *b) override;
    int assign(object *o, object *k, object *v) override;
    int assign_base(object *o, object *k, object *v) override;
    int forall(object *o) override;
    object *fetch(object *o, object *k) override;
    object *fetch_base(object *o, object *k) override;
    int fetch_super(object *o, object *k, object **pv, ici_struct *b) override;
};

} // namespace ici

#endif /* ICI_STRUCT_H */

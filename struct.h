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
    ici_obj_t   *sl_key;
    ici_obj_t   *sl_value;
};

struct ici_struct : objwsup
{
    size_t      s_nels;         /* How many slots used. */
    size_t      s_nslots;       /* How many slots allocated. */
    ici_sslot_t *s_slots;
};

inline ici_struct_t *ici_structof(ici_obj_t *o) { return static_cast<ici_struct_t *>(o); }
inline bool ici_isstruct(ici_obj_t *o)          { return o->isa(ICI_TC_STRUCT); }

/*
 * End of ici.h export. --ici.h-end--
 */

class struct_type : public type
{
public:
    struct_type() : type("struct", sizeof (struct ici_struct), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 forall(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    ici_obj_t *         fetch_base(ici_obj_t *o, ici_obj_t *k) override;
    int                 fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **pv, ici_struct_t *b) override;
};

} // namespace ici

#endif /* ICI_STRUCT_H */

// -*- mode:c++ -*-

#ifndef ICI_PTR_H
#define ICI_PTR_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ptr : object
{
    ici_obj_t   *p_aggr;        /* The aggregate which contains the object. */
    ici_obj_t   *p_key;         /* The key which references it. */
};

inline ici_ptr_t *ici_ptrof(ici_obj_t *o) { return static_cast<ici_ptr_t *>(o); }
inline bool ici_isptr(ici_obj_t *o) { return o->isa(ICI_TC_PTR); }

/*
 * End of ici.h export. --ici.h-end--
 */

class ptr_type : public type
{
public:
    ptr_type() : type("ptr", sizeof (struct ptr), type::has_call) {}

    size_t mark(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int call(ici_obj_t *o, ici_obj_t *subject) override;
};

} // namespace ici

#endif  /* ICI_PTR_H */

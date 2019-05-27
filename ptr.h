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
    object   *p_aggr;        /* The aggregate which contains the object. */
    object   *p_key;         /* The key which references it. */
};

inline ptr *ptrof(object *o) { return o->as<ptr>(); }
inline bool isptr(object *o) { return o->hastype(TC_PTR); }

/*
 * End of ici.h export. --ici.h-end--
 */

class ptr_type : public type
{
public:
    ptr_type() : type("ptr", sizeof (struct ptr), type::has_call) {}

    size_t mark(object *o) override;
    int cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    object *fetch(object *o, object *k) override;
    int assign(object *o, object *k, object *v) override;
    int call(object *o, object *subject) override;
    int save(archiver *, object *) override;
    object *restore(archiver *) override;
};

} // namespace ici

#endif  /* ICI_PTR_H */

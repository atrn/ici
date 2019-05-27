// -*- mode:c++ -*-

#ifndef ICI_FORALL_H
#define ICI_FORALL_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct forall : object
{
    size_t  fa_index;
    object *fa_aggr;
    object *fa_code;
    object *fa_vaggr;
    object *fa_vkey;
    object *fa_kaggr;
    object *fa_kkey;
};

inline forall *forallof(object *o) { return o->as<forall>(); }
inline bool isforall(object *o) { return o->hastype(TC_FORALL); }

/*
 * End of ici.h export. --ici.h-end--
 */
class forall_type : public type
{
public:
    forall_type() : type("forall", sizeof (struct forall)) {}
    size_t mark(object *o) override;
};

} // namespace ici

#endif /* ICI_FORALL_H */

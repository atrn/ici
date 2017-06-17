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
    int         fa_index;
    ici_obj_t   *fa_aggr;
    ici_obj_t   * fa_code;
    ici_obj_t   * fa_vaggr;
    ici_obj_t   * fa_vkey;
    ici_obj_t   * fa_kaggr;
    ici_obj_t   * fa_kkey;
};

inline ici_forall_t *forallof(ici_obj_t *o) { return static_cast<ici_forall_t *>(o); }
inline bool isforall(ici_obj_t *o) { return o->isa(ICI_TC_FORALL); }

/*
 * End of ici.h export. --ici.h-end--
 */
class forall_type : public type
{
public:
    forall_type() : type("forall", sizeof (struct forall)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

} // namespace ici

#endif /* ICI_FORALL_H */

// -*- mode:c++ -*-

#ifndef ICI_PC_H
#define ICI_PC_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct pc : object
{
    array *pc_code;
    object **pc_next;
};

inline pc * ici_pcof(object *o) { return static_cast<pc *>(o); }
inline bool ici_ispc(object *o) { return o->isa(ICI_TC_PC); }

/*
 * End of ici.h export. --ici.h-end--
 */

class pc_type : public type
{
public:
    pc_type() : type("pc", sizeof (struct pc)) {}
    size_t mark(object *o) override;
};


} // namespace ici

#endif  /* ICI_PC_H */

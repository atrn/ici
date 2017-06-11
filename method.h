// -*- mode:c++ -*-

#ifndef ICI_METHOD_H
#define ICI_METHOD_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_method : object
{
    ici_obj_t   *m_subject;
    ici_obj_t   *m_callable;
};

inline ici_method_t *ici_methodof(ici_obj_t *o) { return static_cast<ici_method_t *>(o); }
inline bool ici_ismethod(ici_obj_t *o) { return o->isa(ICI_TC_METHOD); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_CFUNC_H */

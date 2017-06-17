// -*- mode:c++ -*-

#ifndef ICI_METHOD_H
#define ICI_METHOD_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct method : object
{
    ici_obj_t   *m_subject;
    ici_obj_t   *m_callable;
};

inline ici_method_t *ici_methodof(ici_obj_t *o) { return static_cast<ici_method_t *>(o); }
inline bool ici_ismethod(ici_obj_t *o) { return o->isa(ICI_TC_METHOD); }

/*
 * End of ici.h export. --ici.h-end--
 */

class method_type : public type
{
public:
    method_type() : type("method", sizeof (struct method), type::has_objname|type::has_call) {}
    unsigned long mark(ici_obj_t *o) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
    int call(ici_obj_t *o, ici_obj_t *subject) override;
    void objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
};

} // namespace ici

#endif /* ICI_CFUNC_H */

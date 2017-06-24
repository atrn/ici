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
    object   *m_subject;
    object   *m_callable;
};

inline method *methodof(object *o) { return static_cast<method *>(o); }
inline bool ismethod(object *o) { return o->isa(TC_METHOD); }

/*
 * End of ici.h export. --ici.h-end--
 */

class method_type : public type
{
public:
    method_type() : type("method", sizeof (struct method), type::has_objname|type::has_call) {}
    size_t mark(object *o) override;
    object *fetch(object *o, object *k) override;
    int call(object *o, object *subject) override;
    void objname(object *o, char p[objnamez]) override;
};

} // namespace ici

#endif /* ICI_CFUNC_H */

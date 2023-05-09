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
    object *m_subject;
    object *m_callable;
};

inline method *methodof(object *o)
{
    return o->as<method>();
}
inline bool ismethod(object *o)
{
    return o->hastype(TC_METHOD);
}

class method_type : public type
{
public:
    method_type() : type("method", sizeof(struct method), type::has_objname | type::has_call)
    {
    }

    size_t  mark(object *o) override;
    object *fetch(object *o, object *k) override;
    int     call(object *o, object *subject) override;
    void    objname(object *o, char p[objnamez]) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_CFUNC_H */

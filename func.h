// -*- mode:c++ -*-

#ifndef ICI_FUNC_H
#define ICI_FUNC_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct func : object
{
    array       *f_code;    /* The code of this function, atom. */
    array       *f_args;    /* Array of argument names. */
    ici_struct  *f_autos;   /* Prototype struct of autos (incl. args). */
    str         *f_name;    /* Some name for the function (diagnostics). */
    size_t      f_nautos;   /* If !=0, a hint for auto struct alloc. */
};

inline func *funcof(object *o) { return static_cast<func *>(o); }
inline bool isfunc(object *o) {return o->isa(ICI_TC_FUNC); }

/*
 * End of ici.h export. --ici.h-end--
 */

class func_type : public type
{
public:
    func_type() : type("func", sizeof (struct func), type::has_objname | type::has_call) {}

    size_t mark(object *o) override;
    int cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    object *fetch(object *o, object *k) override;
    void objname(object *o, char p[objnamez]) override;
    int call(object *o, object *subject) override;
};

} // namespace ici

#ifndef ICI_CFUNC_H
/*
 * We'd rather not do this, but lots of code expects it. So for backwards
 * compatibility, we'll also include cfunc.h.
 */
#include "cfunc.h"
#endif

#endif /* ICI_FUNC_H */

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
    map         *f_autos;   /* Prototype struct of autos (incl. args). */
    str         *f_name;    /* Some name for the function (diagnostics). */
    size_t      f_nautos;   /* If !=0, a hint for auto struct alloc. */
};

inline func *funcof(object *o) { return o->as<func>(); }
inline bool isfunc(object *o) {return o->isa(TC_FUNC); }

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
    int save(archiver *, object *) override;
    object *restore(archiver *) override;
};

} // namespace ici

#endif /* ICI_FUNC_H */

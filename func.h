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
    ici_array_t     *f_code;    /* The code of this function, atom. */
    ici_array_t     *f_args;    /* Array of argument names. */
    ici_struct_t    *f_autos;   /* Prototype struct of autos (incl. args). */
    ici_str_t       *f_name;    /* Some name for the function (diagnostics). */
    size_t          f_nautos;   /* If !=0, a hint for auto struct alloc. */
};

inline ici_func_t *ici_funcof(ici_obj_t *o) { return static_cast<ici_func_t *>(o); }
inline bool ici_isfunc(ici_obj_t *o) {return o->isa(ICI_TC_FUNC); }

/*
 * End of ici.h export. --ici.h-end--
 */

class func_type : public type
{
public:
    func_type() : type("func", sizeof (struct func), type::has_objname | type::has_call) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    void                objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
    int                 call(ici_obj_t *o, ici_obj_t *subject) override;
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

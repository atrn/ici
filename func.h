// -*- mode:c++ -*-

#ifndef ICI_FUNC_H
#define ICI_FUNC_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_func : object
{
    ici_array_t     *f_code;    /* The code of this function, atom. */
    ici_array_t     *f_args;    /* Array of argument names. */
    ici_struct_t    *f_autos;   /* Prototype struct of autos (incl. args). */
    ici_str_t       *f_name;    /* Some name for the function (diagnostics). */
    int             f_nautos;   /* If !=0, a hint for auto struct alloc. */
};

inline ici_func_t *ici_funcof(ici_obj_t *o) { return static_cast<ici_func_t *>(o); }
inline bool ici_isfunc(ici_obj_t *o) {return o->isa(ICI_TC_FUNC); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#ifndef ICI_CFUNC_H
/*
 * We'd rather not do this, but lots of code expects it. So for backwards
 * compatibility, we'll also include cfunc.h.
 */
#include "cfunc.h"
#endif

#endif /* ICI_FUNC_H */

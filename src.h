// -*- mode:c++ -*-

#ifndef ICI_SRC_H
#define ICI_SRC_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct which is the ICI src object. These are never seen by
 * ICI script code. They are source line markers that are passed to
 * debugger functions to indicate source location.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct src : object
{
    src() : object(ICI_TC_SRC), s_lineno(0), s_filename(nullptr) {}
    int         s_lineno;
    ici_str_t   *s_filename;
};
/*
 * s_filename           The name of the source file this source
 *                      marker is associated with.
 *
 * s_lineno             The linenumber.
 *
 * --ici-api-- continued.
 */
inline ici_src_t *ici_srcof(ici_obj_t *o) { return static_cast<ici_src_t *>(o); }
inline bool ici_issrc(ici_obj_t *o) { return o->isa(ICI_TC_SRC); }

/*
 * End of ici.h export. --ici.h-end--
 */

class src_type : public type
{
public:
    src_type() : type("src", sizeof (struct src)) {}
    size_t mark(ici_obj_t *o) override;
};

} // namespace ici

#endif /* ICI_SRC_H */

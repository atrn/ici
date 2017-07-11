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
 * The struct which is the ICI src object. These are never seen by
 * ICI script code. They are source line markers that are passed to
 * debugger functions to indicate source location.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct src : object
{
    src() : object(TC_SRC, 0, 1), s_lineno(0), s_filename(nullptr) {}

    int s_lineno;
    str *s_filename;
};
/*
 * s_lineno             The linenumber.
 *
 * s_filename           The name of the source file this source
 *                      marker is associated with.
 *
 * --ici-api-- continued.
 */

inline src *srcof(object *o) { return static_cast<src *>(o); }
inline bool issrc(object *o) { return o->isa(TC_SRC); }

/*
 * End of ici.h export. --ici.h-end--
 */

class src_type : public type
{
public:
    src_type() : type("src", sizeof (src)) {}
    size_t mark(object *o) override;
};

} // namespace ici

#endif /* ICI_SRC_H */

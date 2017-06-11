// -*- mode:c++ -*-

#ifndef ICI_MARK_H
#define ICI_MARK_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * Mark objects are used in a few placed in the interpreter when we need
 * an object that we can guarantee is distinct from any object a user
 * could give us. One use is as the 'label' on the default clause of
 * the struct that represents a switch statement.
 */
struct ici_mark : object
{
    ici_mark() : object(ICI_TC_MARK) {}
};

inline ici_mark_t *ici_markof(ici_obj_t *o) { return static_cast<ici_mark_t *>(o); }
inline bool ici_ismark(ici_obj_t *o) { return o == &ici_o_mark; }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_MARK_H */

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
struct ici_mark : ici_obj
{
    ici_mark() : ici_obj(ICI_TC_MARK) {}
};
#define ici_markof(o)       (static_cast<ici_mark_t >)o)
#define ici_ismark(o)       ((o) == ici_objof(&ici_o_mark))

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_MARK_H */

// -*- mode:c++ -*-

#ifndef ICI_CATCH_H
#define ICI_CATCH_H

namespace ici
{

/*
 * catch.h - ICI catch objects. Catch objects are never accesible to ICI
 * programs (and must never be). They mark a point on the execution stack
 * which can be unwound to, and also reveal what depth of operand and
 * scope stack matches that. Associated with a catch object is a "catcher".
 * In a try-onerror catcher this is the code array of the "onerror" clause.
 */

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

struct ici_catch : ici_obj
{
    ici_obj_t   *c_catcher;
    short       c_odepth;       /* Operand stack depth. */
    short       c_vdepth;       /* Variable stack depth. */
};

inline ici_catch_t *ici_catchof(ici_obj_t *o) { return static_cast<ici_catch_t *>(o); }
inline bool ici_iscatch(ici_obj_t *o) { return o->isa(ICI_TC_CATCH); }

/*
 * Flags set stored in the upper nibble of o_flags (which is
 * allowed to be used by objects).
 */
constexpr int CF_EVAL_BASE = 0x20;    /* ici_evaluate should return. */
constexpr int CF_CRIT_SECT = 0x40;    /* Critical section guard. */

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_CATCH_H */

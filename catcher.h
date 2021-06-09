// -*- mode:c++ -*-

#ifndef ICI_CATCHER_H
#define ICI_CATCHER_H

#include "object.h"

namespace ici
{

/*
 * catch.h - ICI catch objects. Catch objects are never accesible to ICI
 * programs (and must never be). They mark a point on the execution stack
 * which can be unwound to, and also reveal what depth of operand and
 * scope stack matches that. Associated with a catch object is a "catcher".
 * In a try-onerror catcher this is the code array of the "onerror" clause.
 */

#include "object.h"

struct catcher : object {
    object  *c_catcher;
    uint32_t c_odepth;       /* Operand stack depth. */
    uint32_t c_vdepth;       /* Variable stack depth. */
};

inline catcher *catcherof(object *o) { return o->as<catcher>(); }
inline bool iscatcher(object *o) { return o->hastype(TC_CATCHER); }

/*
 * Flags set stored in the upper nibble of o_flags (which is
 * allowed to be used by objects).
 */
constexpr int CF_EVAL_BASE = 0x20;    /* evaluate should return. */
constexpr int CF_CRIT_SECT = 0x40;    /* Critical section guard. */

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * End of ici.h export. --ici.h-end--
 */

class catcher_type : public type
{
public:
    catcher_type() : type("catcher", sizeof (struct catcher)) {}
    size_t mark(object *o) override;
    void free(object *o) override;
};

} // namespace ici

#endif /* ICI_CATCHER_H */

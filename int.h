// -*- mode:c++ -*-

#ifndef ICI_INT_H
#define ICI_INT_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 * The C struct that is the ICI int object.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_int : object
{
    int64_t i_value;
};

inline ici_int *ici_intof(object *o) { return static_cast<ici_int *>(o); }
inline bool ici_isint(object *o) { return o->isa(ICI_TC_INT); }

/*
 * End of ici.h export. --ici.h-end--
 */

class int_type : public type
{
public:
    int_type() : type("int", sizeof (struct ici_int)) {}
    int cmp(object *, object *) override;
    unsigned long hash(object *) override;
};

/*
 * So-called "small" integers are pre-created to prime the atom table.
 */
constexpr int small_int_count = 1024;
constexpr int small_int_mask  = 0x3FF;

extern ici_int *small_ints[small_int_count];

} // namespace ici

#endif /* ICI_INT_H */

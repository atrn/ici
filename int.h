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

inline ici_int *intof(object *o) { return o->as<ici_int>(); }
inline bool isint(object *o) { return o->isa(TC_INT); }

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
constexpr int small_int_count = 256;
constexpr int small_int_mask  = 0xFF;

extern ici_int *small_ints[small_int_count];

} // namespace ici

#endif /* ICI_INT_H */

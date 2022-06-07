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
struct integer : object
{
    int64_t i_value;
};

inline integer *intof(object *o)
{
    return o->as<integer>();
}
inline bool isint(object *o)
{
    return o->hastype(TC_INT);
}

class int_type : public type
{
  public:
    int_type()
        : type("int", sizeof(struct integer))
    {
    }
    int           cmp(object *, object *) override;
    unsigned long hash(object *) override;
    int           save(archiver *, object *) override;
    object       *restore(archiver *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * So-called "small" integers are pre-created to prime the atom table.
 */
constexpr int small_int_count = 256;
constexpr int small_int_mask = 0xFF;

extern integer *small_ints[small_int_count];

} // namespace ici

#endif /* ICI_INT_H */

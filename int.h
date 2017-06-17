// -*- mode:c++ -*-

#ifndef ICI_INT_H
#define ICI_INT_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

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
    explicit ici_int(long v = 0)
        : object(ICI_TC_INT)
        , i_value(v)
    {
    }

    int64_t i_value;
};

inline ici_int_t *ici_intof(ici_obj_t *o) { return static_cast<ici_int_t *>(o); }
inline bool ici_isint(ici_obj_t *o) { return o->isa(ICI_TC_INT); }

/*
 * End of ici.h export. --ici.h-end--
 */

class int_type : public type
{
public:
    int_type() : type("int", sizeof (struct ici_int)) {}
    int cmp(ici_obj_t *, ici_obj_t *) override;
    unsigned long hash(ici_obj_t *) override;
};

/*
 * So-called "small" integers are pre-created.
 */
constexpr int ICI_SMALL_INT_COUNT = 1024;
constexpr int ICI_SMALL_INT_MASK  = 0x3FF;

extern ici_int_t *ici_small_ints[ICI_SMALL_INT_COUNT];

} // namespace ici

#endif /* ICI_INT_H */

// -*- mode:c++ -*-

#ifndef ICI_FLOAT_H
#define ICI_FLOAT_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct that is the ICI float object.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_float : object
{
    ici_float(double v = 0.0)
        : object(TC_FLOAT, 0, 1)
        , f_value(v)
    {
    }
    double f_value;
};

inline ici_float *floatof(object *o)
{
    return o->as<ici_float>();
}
inline bool isfloat(object *o)
{
    return o->hastype(TC_FLOAT);
}

class float_type : public type
{
  public:
    float_type()
        : type("float", sizeof(struct ici_float))
    {
    }
    int           cmp(object *o1, object *o2) override;
    unsigned long hash(object *o) override;
    int           save(archiver *, object *) override;
    object       *restore(archiver *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * Are two doubles the same bit pattern? Assumes that doubles are 2 x long
 * size. There are some asserts for this around the place. a and b are
 * pointers to the doubles.
 *
 * We use this when we look up floats in the atom table. We can't just use
 * floating point comparison because the hash is based on the exact bit
 * pattern, and you can get floats that have equal value, but have different
 * bit patterns (0.0 and -0.0 for example). We can end up with different
 * float objects that have apparently equal value. But that's better than
 * having our hash table go bad.
 */
#ifndef __GNUC__
#define DBL_BIT_CMP(a, b) (((int32_t *)(a))[0] == ((int32_t *)(b))[0] && ((int32_t *)(a))[1] == ((int32_t *)(b))[1])
#else
#define DBL_BIT_CMP(a, b) (memcmp(a, b, sizeof(double)) == 0)
#endif

} // namespace ici

#endif /* ICI_FLOAT_H */

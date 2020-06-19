// -*- mode:c++ -*-

#ifndef ICI_VEC_H
#define ICI_VEC_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 *  A 'vec' object is a _vector_, a 1-dimensional array, of an
 *  underlying floating point type - C's float or double.
 *
 *  A 'vec' is created with a defined maximum size, its /capacity/,
 *  and allocates that many elements of the underlyin type when the
 *  vec is created. This array represents the vec's _values_.
 *
 *  A vec's values are accessed by using the vec as an array via
 *  indexing with integer keys.
 *
 *  In addition to values a vec has _properties_. User-defined
 *  key/value pairs (it's an ici map). All non-integer keys are
 *  redirected to the properties map. Typically strings keys are
 *  used permitting ici's dot-notation be used for user-defined
 *  properties.
 *
 *  The data values held in a vec are accessed by indexing the
 *  vec object with integer keys, the value's /offset/ within
 *  the vec array. Both reading (fetch) and writing (assign)
 *  are supported.
 *
 *  A vec object has two important attributes, its 'capacity' and
 *  its 'size'.  The 'capacity' is maximum number of values the
 *  vec can hold and is defined when the vec object is created.
 *
 *  The 'size' is the count of the actual number of values held in the
 *  vec - one more than the maximum index assigned a value.  When
 *  first created a vec has a size of zero. Assigning to elements of
 *  the vec sets its size, up to the vec's capacity. Setting an
 *  element that is NOT the /next/ 
 *
 *  A vec's /length/, as returned by the ICI len() function, is the
 *  vec' 'size'.  The cpacity of a vec, its maximum size, is obtained
 *  by indexing the vec object with the key "capacity".  Assignment to
 *  a vec's "capacity" is not permitted and raises an error.
 *
 *  Reading values from a vec object, via indexing with an integer
 *  key, is limited to reading values up to the vec' size'.  I.e. it
 *  is not possible to read data values that have not been set.
 *
 *  Writing values to a vec object extends its 'size'.  Any value in
 *  the vec, up to its size - 1, may be assigned to.  If the element
 *  being assigned is beyond current size values inbetween the old
 *  and new sizes are set to zero values. Following the assignment to
 *  value at offset i the vec object will have a 'size' of i+1. The
 *  maximum writable position being equal to 'vec.capacity - 1'.
 *
 *  Additionally a vec' 'size' may be retrieved by indexing the
 *  vec object with "size".  Assigning to a vec' "size" IS
 *  permitted and functions in a similar manner as an indexed assignment.
 *  The 'size' may only be set to values from 0 to the vec' capacity
 *  and extending the 'size' beyond its current value writes zeros
 *  to the now accessible values.
 *
 *  Properties
 *
 *  In addition to its array of values a vec object also has a map
 *  used to hold user-defined _properties_.  When a vec object is
 *  indexed by keys other than integers or the strings "size" and
 *  "capacity" it forwards the access to the vec' properties map
 *  allowing users to define, almost, arbitrary collections of
 *  properies to associate with the vec object.
 */
template <int TYPE_CODE, typename VALUE_TYPE>
struct vec : object
{
    constexpr static int type_code  = TYPE_CODE;
    using                value_type = VALUE_TYPE;

    value_type *v_ptr;          // v_capacity x value_type's
    size_t      v_size;         // current length
    size_t      v_capacity;     // total capacity
    map *       v_props;        // user-defined properties
    vec *       v_parent;       // parent vec (iff this is a slice)

    vec(const vec &) = delete;
    vec & operator=(const vec &) = delete;
    vec(vec &&) = delete;

    const value_type * data() const
    {
        return v_ptr;
    }

    value_type * data()
    {
        return v_ptr;
    }

    void resize()
    {
        v_size = v_capacity;
    }

    void resize(size_t z)
    {
        if (z <= v_capacity)
        {
            v_size = z;
        }
    }

#ifdef ICI_VEC_USE_IPP
    void fill(value_type, size_t, size_t);
    void fill(value_type, size_t = 0);
    vec & operator=(value_type);
#else
    void fill(value_type value, size_t ofs, size_t lim)
    {
        for (size_t i = ofs; i < lim; ++i)
        {
            v_ptr[i] = value;
        }
        v_size = lim;
    }

    void fill(value_type value, size_t ofs = 0)
    {
        fill(value, ofs, v_capacity);
    }

    vec & operator=(value_type value)
    {
        fill(value);
        return *this;
    }

#endif

    const value_type & operator[](size_t index) const
    {
        return v_ptr[index];
    }

    value_type & operator[](size_t index)
    {
        return v_ptr[index];
    }

#ifdef ICI_VEC_USE_IPP
# define define_vec_binop_vec(OP)        vec & operator OP (const vec &);
# define define_vec_binop_scalar(OP)     vec & operator OP (value_type);
#else
# define define_vec_binop_vec(OP)                       \
    vec & operator OP (const vec &rhs)                  \
    {                                                   \
        for (size_t index = 0; index < v_size; ++index) \
        {                                               \
            v_ptr[index] OP rhs[index];                 \
        }                                               \
        return *this;                                   \
    }
# define define_vec_binop_scalar(OP)                    \
    vec & operator OP (value_type scalar)               \
    {                                                   \
        for (size_t index = 0; index < v_size; ++index) \
        {                                               \
            v_ptr[index] OP scalar;                     \
        }                                               \
        return *this;                                   \
    }
#endif

    define_vec_binop_vec(+=)
    define_vec_binop_vec(-=)
    define_vec_binop_vec(*=)
    define_vec_binop_vec(/=)

    define_vec_binop_scalar(+=)
    define_vec_binop_scalar(-=)
    define_vec_binop_scalar(*=)
    define_vec_binop_scalar(/=)

#undef define_vec_binop_vec
#undef define_vec_binop_scalar

};

using vec32 =  vec<TC_VEC32, float>;
using vec64 =  vec<TC_VEC64, double>;

inline vec32 * vec32of(object *o) { return static_cast<vec32 *>(o); }
inline bool    isvec32(object *o) { return o->hastype(vec32::type_code); }

vec32 *        new_vec32(size_t, size_t = 0, object * = nullptr);
vec32 *        new_vec32(vec32 *);
vec32 *        new_vec32(vec64 *);
vec32 *        new_vec32(vec32 *, size_t, size_t);

inline vec64 * vec64of(object *o) { return static_cast<vec64 *>(o); }
inline bool    isvec64(object *o) { return o->hastype(vec64::type_code); }

vec64 *        new_vec64(size_t, size_t = 0, object * = nullptr);
vec64 *        new_vec64(vec64 *);
vec64 *        new_vec64(vec32 *);
vec64 *        new_vec64(vec64 *, size_t, size_t);

/*
 * End of ici.h export. --ici.h-end--
 */

// ----------------------------------------------------------------

struct vec32_type : type
{
    vec32_type() : type("vec32", sizeof (vec32)) {}

    size_t mark(object *) override;
    void free(object *) override;
    int64_t len(object *o) override;
    object *copy(object *) override;
    object *fetch(object *o, object *k) override;
    int assign(object *o, object *k, object *v) override;
    int forall(object *) override;
    int save(archiver *, object *) override;
    object * restore(archiver *) override;
};

struct vec64_type : type
{
    vec64_type() : type("vec64", sizeof (vec64)) {}

    size_t mark(object *) override;
    void free(object *) override;
    int64_t len(object *o) override;
    object *copy(object *) override;
    object *fetch(object *o, object *k) override;
    int assign(object *o, object *k, object *v) override;
    int forall(object *) override;
    int save(archiver *, object *) override;
    object * restore(archiver *) override;
};

} // namespace ici

#endif

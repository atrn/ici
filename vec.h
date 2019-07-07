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
 *  A 'vec' object is an 1-dimensional array of an underlying C type -
 *  float or double. A 'vec' is created with a defined maximum size,
 *  or _capacity_, and allocates that many elements of the underlying
 *  type.
 *
 *  The data values held in a vec are accessed by indexing the
 *  vec object with integer keys, the value's _offset_ within
 *  the vec array. Both reading (fetch) and writing (assign)
 *  are supported.
 *
 *  A vec object has two important attributes, its 'capacity' and
 *  its 'size'.  The 'capacity' is maximum number of values the
 *  vec can hold and is defined when the vec object is created.
 *
 *  The 'size' is the count of the actual number of values held in the
 *  vec.  When first created a vec has a size of 0. Adding data to the
 *  vec increases that number, up to the vec's capacity.
 *
 *  A 'vec' _len_, as returned by the ICI len() function, is the
 *  vec' 'size'.  The cpacity of a vec, the maximum _len_, is
 *  obtained by indexing the vec object with the key "capacity".
 *  Attempting to assign to a vec' "capacity" is an error.
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
    typedef VALUE_TYPE value_type;
    constexpr static auto type_code = TYPE_CODE;

    size_t      v_capacity;     // total capacity
    size_t      v_size;         // current length
    value_type *v_ptr;          // v_capacity x value_type's
    map *       v_props;        // user-defined properties

    const value_type * data() const
    {
        return v_ptr;
    }

    value_type * data()
    {
        return v_ptr;
    }

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

    const value_type & operator[](size_t index) const
    {
        return v_ptr[index];
    }

    value_type & operator[](size_t index)
    {
        return v_ptr[index];
    }

    vec & operator=(value_type value)
    {
        fill(value);
        return *this;
    }

#define define_vec_binop(OP)                            \
    vec & operator OP (const vec &rhs)                  \
    {                                                   \
        for (size_t index = 0; index < v_size; ++index) \
        {                                               \
            v_ptr[index] OP rhs[index];                 \
        }                                               \
        return *this;                                   \
    }

    define_vec_binop(+=)
    define_vec_binop(-=)
    define_vec_binop(*=)
    define_vec_binop(/=)

#undef define_vec_binop

#define define_vec_binop_scalar(OP)                     \
    vec & operator OP (value_type scalar)               \
    {                                                   \
        for (size_t index = 0; index < v_size; ++index) \
        {                                               \
            v_ptr[index] OP scalar;                     \
        }                                               \
        return *this;                                   \
    }

    define_vec_binop_scalar(+=)
    define_vec_binop_scalar(-=)
    define_vec_binop_scalar(*=)
    define_vec_binop_scalar(/=)

#undef define_vec_binop_scalar
};

using vec32 =  vec<TC_VEC32, float>;

inline vec32 * vec32of(ici::object *o) { return static_cast<vec32 *>(o); }
inline bool    isvec32(ici::object *o) { return o->hastype(vec32::type_code); }
vec32 *        new_vec32(size_t, size_t = 0, object * = nullptr);

using vec64 =  vec<TC_VEC64, double>;

inline vec64 * vec64of(ici::object *o) { return static_cast<vec64 *>(o); }
inline bool    isvec64(ici::object *o) { return o->hastype(vec64::type_code); }
vec64 *        new_vec64(size_t, size_t = 0, object * = nullptr);

/*
 * End of ici.h export. --ici.h-end--
 */

// ----------------------------------------------------------------

struct vec32_type : ici::type
{
    vec32_type() : ici::type("vec32", sizeof (vec32)) {}

    size_t mark(ici::object *) override;
    void free(ici::object *) override;
    int64_t len(ici::object *o) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
    int assign(ici::object *o, ici::object *k, ici::object *v) override;
    int forall(object *) override;
    int save(archiver *, object *) override;
    object * restore(archiver *) override;
};

struct vec64_type : ici::type
{
    vec64_type() : ici::type("vec64", sizeof (vec64)) {}

    size_t mark(ici::object *) override;
    void free(ici::object *) override;
    int64_t len(ici::object *o) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
    int assign(ici::object *o, ici::object *k, ici::object *v) override;
    int forall(object *) override;
    int save(archiver *, object *) override;
    object * restore(archiver *) override;
};

} // namespace ici

#endif

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
 *  A 'vec' object is an 1-dimensional array of an underlying C
 *  type - float or double. A 'vec' is created with a defined
 *  maximum size and allocates that many elements of the underlying
 *  type.
 *
 *  The data values held in a vec are accessed by indexing the
 *  vec object with integer keys, the value's _offset_ within
 *  the vec array. Both reading (fetch) and writing (assign)
 *  are supported.
 *
 *  A vec object has two important attributes, its 'size' and what
 *  is called its 'count'.  The 'size' is maximum number of values able
 *  to be held in the vec and is defined when the vec object
 *  is created.
 *
 *  The 'count' counts the actual number of values held in the vec.
 *  When a vec object is first created it has a count of 0. Adding
 *  data to the vec increases the count, up to the vec size.
 *
 *  A 'vec' _len_, as returned by the ICI len() function, is the
 *  vec' 'count'.  The size of a vec, its maximum _len_, is
 *  obtained by indexing the vec object with the key "size".
 *  Attempting to assign to a vec' "size" is an error.
 *
 *  Reading values from a vec object, via indexing with an integer
 *  key, is limited to reading values up to the vec' count'.
 *  I.e. it is not possible to read data values that have not been
 *  set.
 *
 *  Writing values to a vec object extends its 'count'.  Any value
 *  in the vec, up to its size, may be assigned a numeric value.
 *  If the value being assigned is beyond of the vec' current count
 *  values between the 'count' and the new value are filled with zero
 *  values. Following the assignment to value at offset i the vec
 *  object will have a 'count' of i+1. The maximum writable position
 *  being equal to 'vec.size - 1'.
 *
 *  Additionally a vec' 'count' may be retrieved by indexing the
 *  vec object with "count".  Assigning to a vec' "count" IS
 *  permitted and functions in a similar manner to indexed assignment.
 *  The 'count' may only be set to values from 0 to the vec' size
 *  and extending the 'count' beyonds its current value writes zeros
 *  to the now accessible values.
 *
 *  Properties
 *
 *  In addition to its array of values a vec object also has a map
 *  used to hold user-defined _properties_.  When a vec object is
 *  indexed by keys other than integers or the strings "size" and
 *  "count" it forwards the access to the vec' properties map
 *  allowing users to define, almost, arbitrary collections of
 *  properies to associate with the vec object.
 */
template <int TYPE_CODE, typename VALUE_TYPE>
struct vec : object
{
    typedef VALUE_TYPE value_type;
    constexpr static auto type_code = TYPE_CODE;

    size_t      _size;          // total capacity
    size_t      _count;         // current length
    value_type *_ptr;           // _size x value_type's
    map *       _props;         // user-defined properties

    const value_type * data() const
    {
        return _ptr;
    }

    value_type * data()
    {
        return _ptr;
    }

    void fill(value_type value)
    {
        for (size_t i = 0; i < _size; ++i)
        {
            _ptr[i] = value;
        }
        _count = _size;
    }

    const value_type & operator[](size_t index) const
    {
        return _ptr[index];
    }

    value_type & operator[](size_t index)
    {
        return _ptr[index];
    }

    vec & operator+=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] += scalar;
        }
        return *this;
    }

    vec & operator-=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] -= scalar;
        }
        return *this;
    }

    vec & operator*=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] *= scalar;
        }
        return *this;
    }

    vec & operator/=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] /= scalar;
        }
        return *this;
    }
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
    static inline vec32 * cast(object *o) { return static_cast<vec32 *>(o); }

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
    static inline vec64 * cast(object *o) { return static_cast<vec64 *>(o); }

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

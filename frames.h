// -*- mode:c++ -*-

#ifndef ICI_FRAMES_H
#define ICI_FRAMES_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 *  A 'frames' object is an 1-dimensional array of an underlying C
 *  type - float or double. A 'frames' is created with a defined
 *  maximum size and allocates that many elements of the underlying
 *  type.
 *
 *  The data values held in a frames are accessed by indexing the
 *  frames object with integer keys, the value's _offset_ within
 *  the frames array. Both reading (fetch) and writing (assign)
 *  are supported.
 *
 *  A frames object has two important attributes, its 'size' and what
 *  is called its 'count'.  The 'size' is maximum number of values able
 *  to be held in the frame and is defined when the frames object
 *  is created.
 *
 *  The 'count' counts the actual number of values held in the frame.
 *  When a frames object is first created it has a count of 0. Adding
 *  data to the frame increases the count, up to the frames size.
 *
 *  A 'frames' _len_, as returned by the ICI len() function, is the
 *  frames' 'count'.  The size of a frames, its maximum _len_, is
 *  obtained by indexing the frames object with the key "size".
 *  Attempting to assign to a frames' "size" is an error.
 *
 *  Reading values from a frames object, via indexing with an integer
 *  key, is limited to reading values up to the frames' count'.
 *  I.e. it is not possible to read data values that have not been
 *  set.
 *
 *  Writing values to a frames object extends its 'count'.  Any value
 *  in the frames, up to its size, may be assigned a numeric value.
 *  If the value being assigned is beyond of the frames' current count
 *  values between the 'count' and the new value are filled with zero
 *  values. Following the assignment to value at offset i the frames
 *  object will have a 'count' of i+1. The maximum writable position
 *  being equal to 'frames.size - 1'.
 *
 *  Additionally a frames' 'count' may be retrieved by indexing the
 *  frames object with "count".  Assigning to a frames' "count" IS
 *  permitted and functions in a similar manner to indexed assignment.
 *  The 'count' may only be set to values from 0 to the frames' size
 *  and extending the 'count' beyonds its current value writes zeros
 *  to the now accessible values.
 *
 *  Properties
 *
 *  In addition to its array of values a frames object also has a map
 *  used to hold user-defined _properties_.  When a frames object is
 *  indexed by keys other than integers or the strings "size" and
 *  "count" it forwards the access to the frames' properties map
 *  allowing users to define, almost, arbitrary collections of
 *  properies to associate with the frames object.
 */
template <int TYPE_CODE, typename VALUE_TYPE>
struct frames : object
{
    typedef VALUE_TYPE value_type;
    constexpr static auto type_code = TYPE_CODE;

    size_t      _size;          // total capacity
    size_t      _count;         // current length
    value_type *_ptr;           // pointer to _size value_type's
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

    frames & operator+=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] += scalar;
        }
        return *this;
    }

    frames & operator-=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] -= scalar;
        }
        return *this;
    }

    frames & operator*=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] *= scalar;
        }
        return *this;
    }

    frames & operator/=(value_type scalar)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            _ptr[index] /= scalar;
        }
        return *this;
    }
};

using frames32 = frames<TC_FRAMES32, float>;

inline frames32 *frames32of(ici::object *o) { return static_cast<frames32 *>(o); }
inline bool      isframes32(ici::object *o) { return o->hastype(frames32::type_code); }
frames32 *       new_frames32(size_t, size_t = 0, object * = nullptr);

using frames64 = frames<TC_FRAMES64, double>;

inline frames64 *frames64of(ici::object *o) { return static_cast<frames64 *>(o); }
inline bool      isframes64(ici::object *o) { return o->hastype(frames64::type_code); }
frames64 *       new_frames64(size_t, size_t = 0, object * = nullptr);

/*
 * End of ici.h export. --ici.h-end--
 */

struct frames32_type : ici::type
{
    frames32_type() : ici::type("frames32", sizeof (frames32)) {}

    size_t mark(ici::object *) override;
    void free(ici::object *) override;
    int64_t len(ici::object *o) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
    int assign(ici::object *o, ici::object *k, ici::object *v) override;
    int forall(object *) override;
    int save(archiver *, object *) override;
    object * restore(archiver *) override;
};

struct frames64_type : ici::type
{
    frames64_type() : ici::type("frames64", sizeof (frames64)) {}

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

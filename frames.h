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
 *  A frames has a 'count' intended to _count_ the amount of data in the frame.
 *
 *  The size of a frames may be obtained by indexing the frames
 *  object with the key "size".
 *
 *  A 'frames' _len_, as returned by the ICI len() function, is
 *  the count of the frames.
 *
 *  Access to a frame's data is performed by indexing the frame
 *  using integer keys (array indices).
 *
 *  Other non-integer keys, usually strings, are used to access
 *  user-defined _properties_ held in an associated map object.
 */
template <typename DATA>
struct frames : object
{
    size_t      _size;          // number of floats
    size_t      _count;         // user-defined 'length'
    DATA *     _ptr;           // pointer to _size floats
    map *       _props;         // user-defined "properties"

    const DATA * data() const
    {
        return _ptr;
    }

    DATA * data()
    {
        return _ptr;
    }

    const DATA & operator[](size_t index) const
    {
        return _ptr[index];
    }

    DATA & operator[](size_t index)
    {
        return _ptr[index];
    }
};

using frames32 = frames<float>;
inline frames32 *frames32of(ici::object *o) { return static_cast<frames32 *>(o); }
inline bool isframes32(ici::object *o)  { return o->hastype(TC_FRAMES32); }
frames32 * new_frames32(size_t);

using frames64 = frames<double>;
inline frames64 *frames64of(ici::object *o) { return static_cast<frames64 *>(o); }
inline bool isframes64(ici::object *o)  { return o->hastype(TC_FRAMES64); }
frames64 * new_frames64(size_t);

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
};

struct frames64_type : ici::type
{
    frames64_type() : ici::type("frames64", sizeof (frames64)) {}

    size_t mark(ici::object *) override;
    void free(ici::object *) override;
    int64_t len(ici::object *o) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
    int assign(ici::object *o, ici::object *k, ici::object *v) override;
};

} // namespace ici

#endif

#ifndef sndfile_frames_h
#define sndfile_frames_h

#include <ici.h>

/*
 *  A 'frames' object is a 1-dimensional array of float.
 */
struct frames : ici::object
{
    size_t      _size;          // number of floats
    size_t      _count;         // user-defined 'length'
    float *     _ptr;           // pointer to _size floats
    int         _samplerate;    // user-defined sample rate
    int         _channels;      // number of channels/frame

    const float * data() const
    {
        return _ptr;
    }

    float * data()
    {
        return _ptr;
    }

    const float & operator[](size_t index) const
    {
        return _ptr[index];
    }

    float & operator[](size_t index)
    {
        return _ptr[index];
    }
};

struct frames_type : ici::type
{
    static int code;

    frames_type() : ici::type("frames", sizeof (frames)) {}

    size_t      mark(ici::object *) override;
    void        free(ici::object *) override;
    int64_t     len(ici::object *o) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
    int         assign(ici::object *o, ici::object *k, ici::object *v) override;
};

inline frames *framesof(ici::object *o) { return static_cast<frames *>(o); }
inline bool isframes(ici::object *o)  { return o->hastype(frames_type::code); }

frames *new_frames(size_t, int, int);

#endif // sndfile_frames_h

#ifndef sndfile_frames_h
#define sndfile_frames_h

#include <ici.h>

struct frames : ici::object
{
    size_t      _size;
    size_t      _count;
    float *     _ptr;
    int         _samplerate;
    int         _channels;

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
inline bool isframes(ici::object *o)  { return o->isa(frames_type::code); }

frames *new_frames(size_t, int, int);

#endif // sndfile_frames_h

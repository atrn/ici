#include "frames.h"
#include "icistr.h"

int frames_type::code = 0;

size_t frames_type::mark(ici::object *o)
{
    return ici::type::mark(o) + framesof(o)->_size;
}

void frames_type::free(ici::object *o)
{
    ici::ici_free(framesof(o)->_ptr);
    ici::type::free(o);
}

int64_t frames_type::len(ici::object *o)
{
    return framesof(o)->_count;
}

ici::object *frames_type::fetch(ici::object *o, ici::object *k)
{
    auto f = framesof(o);
    if (ici::isint(k))
    {
        auto ofs = ici::intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->_count)
        {
            ici::set_error("index out of range");
            return nullptr;
        }
        return ici::new_float((*f)[ofs]);
    }
    if (k == ICISO(count))
    {
        return ici::new_int(f->_count);
    }
    if (k == ICISO(samplerate))
    {
        return ici::new_int(f->_samplerate);
    }
    if (k == ICISO(channels))
    {
        return ici::new_int(f->_channels);
    }
    if (k == ICISO(size))
    {
        return ici::new_int(f->_size);
    }
    return ici::type::fetch(o, k);
}

int frames_type::assign(ici::object *o, ici::object *k, ici::object *v)
{
    auto f = framesof(o);

    if (ici::isint(k))
    {
        auto ofs = ici::intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->_size)
        {
            return ici::set_error("index out of range");
        }
        for (; f->_count < static_cast<size_t>(ofs); ++f->_count)
        {
            (*f)[f->_count] = 0.0f;
        }
        if (ici::isint(v))
        {
            (*f)[ofs] = static_cast<float>(ici::intof(v)->i_value);
            return 0;
        }
        else if (ici::isfloat(v))
        {
            (*f)[ofs] = static_cast<float>(ici::floatof(v)->f_value);
            return 0;
        }
    }

    if (k == ICISO(count))
    {
        if (ici::isint(v))
        {
            auto count = ici::intof(v)->i_value;
            if (count < 0 || static_cast<size_t>(count) > f->_size)
            {
                return ici::set_error("count out of range");
            }
            f->_count = count;
            return 0;
        }
    }
    
    if (k == ICISO(samplerate))
    {
        if (ici::isint(v))
        {
            auto samplerate = ici::intof(v)->i_value;
            if (samplerate <= 0)
            {
                return ici::set_error("invalid sample rate");
            }
            f->_samplerate = samplerate;
            return 0;
        }
    }

    if (k == ICISO(channels))
    {
        if (ici::isint(v))
        {
            auto channels = ici::intof(v)->i_value;
            if (channels <= 0)
            {
                return ici::set_error("invalid channel count");
            }
            f->_channels = channels;
            return 0;
        }
    }

    return ici::type::assign(o, k, v);
}

frames *new_frames(size_t nframes, int samplerate, int channels)
{
    const auto z = nframes * channels;
    auto f = ici::ici_talloc<frames>();
    if (f)
    {
        f->_ptr = static_cast<float *>(ici::ici_alloc(z * sizeof (float)));
        if (!f->_ptr)
        {
            ici::ici_free(f);
            return nullptr;
        }
        f->set_tfnz(frames_type::code, 0, 1, 0);
        f->_size = z;
        f->_count = 0;
        f->_samplerate = samplerate;
        f->_channels = channels;
        ici::rego(f);
    }
    return f;
}

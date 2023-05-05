#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include "file.h"

namespace
{

bool get_int(ici::vec32f *vec, ici::object *k, int64_t &value)
{
    if (const auto v = vec->fetch(k))
    {
        if (ici::isint(v))
        {
            value = intof(v)->i_value;
            return true;
        }
        ici::set_error("input vector '%s' attribute is not an integer", ici::stringof(k)->s_chars);
        return false;
    }
    ici::set_error("input vector has no '%s' attribute", ici::stringof(k)->s_chars);
    return false;
}

bool set_int(ici::vec32f *vec, ici::object *k, int64_t value)
{
    if (auto v = ici::make_ref<>(ici::new_int(value)))
    {
        return vec->assign(k, v) == 0;
    }
    return false;
}

bool set_properties(ici::vec32f *vec, int64_t samplerate, int64_t channels, int64_t frames)
{
    return
        set_int(vec, ICIS(samplerate), samplerate)
        &&
        set_int(vec, ICIS(channels), channels)
        &&
        set_int(vec, ICIS(frames), frames)
        ;
}

/**
 *  file = sndfile.open(path)
 */
int f_open()
{
    char *path;

    if (ici::typecheck("s", &path))
    {
        return 1;
    }
    auto sf = ici::make_ref(new_sndfile());
    if (!sf)
    {
        return 1;
    }
    if (!sf->open(path, SFM_READ))
    {
        return 1;
    }
    return ici::ret_no_decref(sf);
}


/**
 *  file = sndfile.create(path, format, samplerate, channels)
 */
int f_create()
{
    char *  path;
    int64_t format;
    int64_t samplerate;
    int64_t channels;

    if (ici::typecheck("siii", &path, &format, &samplerate, &channels))
    {
        return 1;
    }

    auto sf = ici::make_ref(new_sndfile());
    if (!sf)
    {
        return 1;
    }

    sf->_info.frames = 0;
    sf->_info.sections = 0;
    sf->_info.seekable = 0;
    sf->_info.format = format;
    sf->_info.samplerate = samplerate;
    sf->_info.channels = channels;

    sf->_file = sf_open(path, SFM_WRITE, &sf->_info);
    if (!sf->_file)
    {
        return sndfile::error();
    }

    return ici::ret_no_decref(sf);
}

/**
 *  frames = sndfile.read(file [, count])
 */
int f_read()
{
    sndfile *sf;
    int64_t numframes = -1;
    
    if (ici::NARGS() == 1)
    {
        if (ici::typecheck("o", &sf))
        {
            return 1;
        }
    }
    else if (ici::typecheck("oi", &sf, &numframes))
    {
        return 1;
    }
    if (!issndfile(sf))
    {
        return ici::argerror(0);
    }
    if (!sf->_file)
    {
        return ici::set_error("attempt to used closed file");
    }
    if (numframes < 0)
    {
        numframes = sf->_info.frames;
    }
    auto data = ici::make_ref(ici::new_vec32f(numframes * sf->_info.channels));
    if (!data)
    {
        return 1;
    }

    numframes = sf_readf_float(sf->_file, data->v_ptr, numframes);
    if (numframes < 0)
    {
        return sndfile::error();
    }
    if (!set_properties(data, sf->_info.samplerate, sf->_info.channels, numframes))
    {
        return 1;
    }
    data->resize(numframes * sf->_info.channels);
    return ici::ret_no_decref(data);
}


/**
 * int = sndfile.write(file, data)
 * int = sndfile.write(file, data, numframes)
 */
int f_write()
{
    sndfile *sf;
    ici::vec32f *data;
    int64_t numframes = -1;

    if (ici::NARGS() == 2)
    {
        if (ici::typecheck("oo", &sf, &data))
        {
            return 1;
        }
    }
    else if (ici::typecheck("oo", &sf, &data, &numframes))
    {
            return 1;
    }

    if (!issndfile(sf))
    {
        return ici::argerror(0);
    }
    if (!ici::isvec32f(data))
    {
        return ici::argerror(1);
    }
    if (!sf->_file)
    {
        return ici::set_error("attempt to used closed file");
    }

    const auto numchannels = sf->_info.channels;
    if (numframes < 0)
    {
        numframes = data->v_size / numchannels;
    }
    else if (numframes * numchannels > int64_t(data->v_size))
    {
        return ici::set_error("frame count exceeds data size");
    }

    const auto r = sf_writef_float(sf->_file, data->v_ptr, numframes);
    return ici::int_ret(r);
}


/*
 * int = sndfile.close(file)
 */
int f_close()
{
    sndfile *sf;

    if (ici::typecheck("o", &sf))
    {
        return 1;
    }
    if (!issndfile(sf))
    {
        return ici::argerror(0);
    }
    return ici::int_ret(sf->close());
}


/**
 * vec = sndfile.channel(vec, index)
 *
 * Return a new vec comprising the samples of the given channel.
 * Channels are numered from 1 on.
 */
int f_channel()
{
    ici::object *vec;
    int64_t     channel;

    if (typecheck("oi", &vec, &channel))
    {
        return 1;
    }
    if (!ici::isvec32f(vec))
    {
        return ici::argerror(0);
    }
    if (channel <= 0)
    {
        return ici::argerror(1);
    }

    int64_t channels;
    int64_t samplerate;
    int64_t frames;

    if (!get_int(ici::vec32fof(vec), ICIS(channels), channels))
    {
        return 1;
    }
    if (!get_int(ici::vec32fof(vec), ICIS(samplerate), samplerate))
    {
        return 1;
    }
    if (!get_int(ici::vec32fof(vec), ICIS(frames), frames))
    {
        return 1;
    }

    const auto size = size_t(ceil(ici::vec32fof(vec)->v_size / double(channels)));
    auto result = ici::make_ref(ici::new_vec32f(size));
    if (!result)
    {
        return 1;
    }

    for (size_t i = 0, j = 0; j < size; i += channels, ++j)
    {
        ici::vec32fof(result)->v_ptr[j] = ici::vec32fof(vec)->v_ptr[i+channel-1];
    }

    ici::vec32fof(result)->resize(size);

    if (!set_properties(result, samplerate, 1, frames))
    {
        return 1;
    }
    return ici::ret_no_decref(result);
}

/*
 * vec = sndfile.combine(vec...)
 *
 * Merge the data from two or more input vecs to interleave
 * their values, i.e. a complement function to channel()
 *
 * Each input vector must have a single channel and have the
 * same sample rate.
 *
 * Vectors of unequal sizes are permitted with zero valued samples
 * substituted for missing data.
 *
 * The returned vector has a "channels" properties equal to the
 * number of input vectors.
 * 
 */
int f_combine()
{
    auto not_one_channel = [](int index)
    {
        return ici::set_error("input vector %d is not a single channel vector", index);
    };

    auto num_channels = [](ici::vec32f *vec) -> int64_t
    {
        int64_t channels;
        if (!get_int(vec, ICIS(channels), channels))
        {
            channels = 0;
        }
        return channels;
    };

    auto get_samplerate = [](ici::vec32f *vec) -> int64_t
    {
        int64_t samplerate;
        if (!get_int(vec, ICIS(samplerate), samplerate))
        {
            samplerate = 0;
        }
        return samplerate;
    };

    if (ici::NARGS() == 1)
    {
        if (!ici::isvec32f(ici::ARG(0)))
        {
            return ici::argerror(0);
        }
        if (num_channels(ici::vec32fof(ici::ARG(0))) != 1)
        {
            return not_one_channel(0);
        }
	return ici::ret_with_decref(ici::ARG(0));
    }

    int64_t samplerate;
    if (!get_int(ici::vec32fof(ici::ARG(0)), ICIS(samplerate), samplerate))
    {
        return 1;
    }

    size_t size = ici::vec_size(ici::ARG(0));
    size_t maxsize = size;

    for (int i = 1; i < ici::NARGS(); ++i)
    {
        if (!ici::isvec32f(ici::ARG(i)))
        {
            return ici::argerror(i);
        }
        if (num_channels(ici::vec32fof(ici::ARG(i))) != 1)
        {
            return not_one_channel(i);
        }
        const auto r = get_samplerate(ici::vec32fof(ici::ARG(i)));
        if (r != samplerate)
        {
            return ici::set_error("sample rate of input vector %d (%lld), differs from input vector 0 (%lld)", i, r, samplerate);
        }
        const auto z = ici::vec_size(ici::ARG(i));
        if (z > maxsize)
        {
            maxsize = z;
        }
        const auto newsize = size + z;
        if (newsize < size) // wrapped
        {
            return ici::set_errorc("combined vec object too large");
        }
        size = newsize;
    }

    auto result = ici::make_ref(ici::new_vec32f(size));
    if (!result)
    {
        return 1;
    }

    size_t j = 0;
    for (size_t i = 0; i < maxsize; ++i)
    {
        for (size_t k = 0; k < size_t(ici::NARGS()); ++k)
        {
            auto v = ici::vec32fof(ici::ARG(k));
            if (i < v->size())
            {
                (*result)[j] = (*v)[i];
            }
            else
            {
                (*result)[j] = 0.0f;
            }
            ++j;
        }
    }
    result->resize(j);

    if (!set_properties(result, samplerate, ici::NARGS(), maxsize))
    {
        return 1;
    }

    return ici::ret_no_decref(result);
}

} // anon


//  ----------------------------------------------------------------

//  Module entry point.  When a module is loaded ICI calls its init
//  function to obtain whatever object the module exports.
//
//  In our case we're a typical module and use ici::new_module() to
//  create our object, a map, containing our cfunc objects.
//
extern "C" ici::object *ici_sndfile_init()
{
    static sndfile_type sndfile_type;

    if (ici::check_interface(ici::version_number, ici::back_compat_version, "sndfile"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    if (!(sndfile_type::code = ici::register_type(&sndfile_type)))
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(sndfile)
    {
        ICI_DEFINE_CFUNC(close, f_close),
        ICI_DEFINE_CFUNC(combine, f_combine),
        ICI_DEFINE_CFUNC(create, f_create),
        ICI_DEFINE_CFUNC(open, f_open),
        ICI_DEFINE_CFUNC(read, f_read),
        ICI_DEFINE_CFUNC(write, f_write),
        ICI_DEFINE_CFUNC(channel, f_channel),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(sndfile));
}

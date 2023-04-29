#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include "file.h"

namespace
{


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
    auto sf = new_sndfile();
    if (!sf)
    {
        return 1;
    }
    if (!sf->open(path, SFM_READ))
    {
        return 1;
    }
    return ici::ret_with_decref(sf);
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

    auto sf = ici::ref<sndfile>(new_sndfile());
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
    auto data = ici::new_vec32f(numframes * sf->_info.channels);
    if (!data)
    {
        return 1;
    }

    numframes = sf_readf_float(sf->_file, data->v_ptr, numframes);
    if (numframes < 0)
    {
        return sndfile::error();
    }

    auto set_int = [&data](ici::object *k, int64_t value)
    {
        if (auto v = ici::make_ref<>(ici::new_int(value)))
        {
            return data->assign(k, v);
        }
        return 1;
    };

    if (set_int(ICIS(samplerate), sf->_info.samplerate))
    {
        return 1;
    }
    if (set_int(ICIS(channels), sf->_info.channels))
    {
        return 1;
    }
    if (set_int(ICIS(frames), numframes))
    {
        return 1;
    }
    data->resize(numframes * sf->_info.channels);
    return ici::ret_with_decref(data);
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
        ICI_DEFINE_CFUNC(create, f_create),
        ICI_DEFINE_CFUNC(open, f_open),
        ICI_DEFINE_CFUNC(read, f_read),
        ICI_DEFINE_CFUNC(write, f_write),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(sndfile));
}

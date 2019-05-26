// Note, not called sndfile.h to avoid clash with libsndfile

#include "file.h"
#include "icistr.h"
#include <cstring>

bool sndfile::open(const char *path, int mode)
{
    memset(&_info, 0, sizeof _info);
    _file = sf_open(path, mode, &_info);
    if (!_file)
    {
        error();
        return false;
    }
    return true;
}

int sndfile::close()
{
    int r = 0;
    if (_file)
    {
        r = sf_close(_file);
        _file = nullptr;
    }
    return r;
}

int sndfile::error(SNDFILE *f)
{
    return ici::set_error("%s", sf_strerror(f));
}

//  ----------------------------------------------------------------

int sndfile_type::code;

void sndfile_type::free(ici::object *o)
{
    sndfileof(o)->close();
    ici::type::free(o);
}

ici::object *sndfile_type::fetch(ici::object *o, ici::object *k)
{
    auto sf = sndfileof(o);
    auto field = [](int64_t v)
    {
        return ici::new_int(v);
    };
    if (k == ICISO(frames))
    {
        return field(sf->_info.frames);
    }
    if (k == ICISO(samplerate))
    {
        return field(sf->_info.samplerate);
    }
    if (k == ICISO(channels))
    {
        return field(sf->_info.channels);
    }
    if (k == ICISO(format))
    {
        return field(sf->_info.format);
    }
    if (k == ICISO(sections))
    {
        return field(sf->_info.sections);
    }
    if (k == ICISO(seekable))
    {
        return field(sf->_info.seekable);
    }
    return ici::type::fetch(o, k);
}
        
//  ----------------------------------------------------------------

sndfile *new_sndfile()
{
    auto sf = ici::ici_talloc<sndfile>();
    if (sf)
    {
        sf->set_tfnz(sndfile_type::code, 0, 1, 0);
        ici::rego(sf);
    }
    return sf;
}

// Note, not called sndfile.h to avoid clash with libsndfile

#ifndef sndfile_file_h
#define sndfile_file_h

#include <ici.h>
#include <sndfile.h>

struct sndfile : ici::object
{
    static int error(SNDFILE * = nullptr);

    SNDFILE *   _file;
    SF_INFO     _info;

    bool open(const char *, int);
    int close();
};

struct sndfile_type : ici::type
{
    static int code;

    sndfile_type() : ici::type("sndfile", sizeof (struct sndfile)) {}

    void free(ici::object *) override;
    ici::object *fetch(ici::object *o, ici::object *k) override;
};

inline sndfile *sndfileof(ici::object *o) { return o->as<sndfile>(); }
inline bool issndfile(ici::object *o) { return o->isa(sndfile_type::code); }

sndfile *new_sndfile();

#endif // sndfile_file_h

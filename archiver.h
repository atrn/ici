// -*- mode:c++ -*-

/*
 * ici object serialization
 *
 * Copyright (C) 1995 A. Newman
 */

#ifndef ICI_ARCHIVE_H
#define ICI_ARCHIVE_H

#include "file.h"
#include "object.h"

namespace ici
{

int  archive_init();
void archive_uninit();
int  f_archive_save(...);
int  f_archive_restore(...);

// HACK - endianess support needs re-think
#if defined(__i386__) || defined(__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * The top bit of the tcode is set when the object is atomic.  This of
 * course limits archiving to 128 distinct types.
 */
constexpr int O_ARCHIVE_ATOMIC = 0x80;

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * An archiver holds the /state/ used when saving or restoring an object graph.
 */
class archiver
{
  public:
    static int op_func_code(int (*fn)());
    static int (*op_func(int))();

    /**
     *  Constructs an archiver that will use the given file and scope.
     *
     *  If saving the file must be writable and if restoring it must
     *  be readable.
     *
     *  The scope is used when restoring function objects. The scope
     *  is the, restored, function's 'local' scope (the super of its
     *  autos) and holds the restored function, keyed by its name.
     *  The object (a map) must be writable.
     */
    archiver(file *, objwsup *);
    operator bool() const; // checks construction success

    ~archiver() = default;
    archiver(archiver &&) = delete;
    archiver(const archiver &) = delete;
    archiver &operator=(const archiver &) = delete;

    inline objwsup *scope() const
    {
        return a_scope;
    }

    /*
     *  Save the given object to the archiver's file.
     *
     *  Returns 0 on success, 1 on error, usual conventions.
     */
    int save(object *);

    /*
     *  Restore an object from the archiver's files.
     *
     *  Returns nullptr on error.
     */
    object *restore();

    inline int read(void *buf, int len)
    {
        return a_file->read(buf, len) != len;
    }

    inline int read(uint8_t *abyte)
    {
        return read(abyte, 1);
    }

    inline int write(const void *buf, int len)
    {
        return a_file->write(buf, len) != len;
    }

    inline int write(uint8_t abyte)
    {
        return write(&abyte, 1);
    }

    int read(int16_t *);
    int read(int32_t *);
    int read(int64_t *);
    int read(float *);
    int read(double *);

    int write(int16_t);
    int write(int32_t);
    int write(int64_t);
    int write(float);
    int write(double);

    int     record(object *, object *);
    object *lookup(object *);
    void    remove(object *);
    int     save_name(object *);
    int     restore_name(object **);
    int     save_ref(object *);
    object *restore_ref();
    int     push_name(str *);
    int     pop_name();
    str    *name_qualifier();

  private:
    file      *a_file;
    objwsup   *a_scope;
    ref<map>   a_sent; // object -> 'name' (int)
    ref<array> a_names;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

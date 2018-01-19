// -*- mode:c++ -*-

/*
 * ici object serialization
 *
 * Copyright (C) 1995 A. Newman
 */

#ifndef ICI_ARCHIVE_H
#define ICI_ARCHIVE_H

#include "object.h"
#include "file.h"

namespace ici
{

int archive_init();
void archive_uninit();

int f_archive_save(...);
int f_archive_restore(...);

#if defined (__i386__) || defined (__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * Bit of the tcode set when the object is atomic.
 */
constexpr int O_ARCHIVE_ATOMIC = 0x80;

long long ici_ntohll(long long v);
long long ici_htonll(long long v);

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * An archiving session.
 */
class archiver
{
public:
    static int op_func_code(int (*fn)());
    static int (*op_func(int))();

    archiver(file *, objwsup *);
    operator bool() const { return a_sent != nullptr; } // check construction sucess
    virtual ~archiver();

    archiver(const archiver &) = delete;
    archiver& operator=(const archiver &) = delete;

    inline objwsup *scope() const { return a_scope; }

    int save(object *);
    object *restore();

    int record(object *, object *);
    object *lookup(object *);
    void remove(object *);

    int save_name(object *);
    int save_ref(object *);
    int restore_name(object **);

    virtual int read(void *buf, int len);
    virtual int write(const void *, int);

    inline int read(char *abyte) {
        return read(abyte, 1);
    }

    int write(unsigned char abyte) {
        return write(&abyte, 1);
    }

    template <typename T>
    int read(T &ref) {
        if (read(&ref, sizeof (T))) {
            return 1;
        }
        return 0;
    }

    int read(int16_t *hword);
    int read(int32_t *aword);
    int read(int64_t *dword);
    int read(double *dbl);

    int write(int16_t hword);
    int write(int32_t aword);
    int write(int64_t dword);
    int write(double adbl);

private:
    file *  a_file;
    map *   a_sent;
    objwsup *a_scope;

private:
    int get() {
        char c;
        if (read(&c) == 1) {
            return c;
        }
        return -1;
    }

    static void byteswap(void *ptr, int sz);
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
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

int archive_op_func_code(int (*fn)());
int (*archive_op_func(int))();

#if defined (__i386__) || defined (__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * Bit of the tcode set when the object is atomic.
 */
constexpr int O_ARCHIVE_ATOMIC = 0x80;

long long ici_ntohll(long long v);
long long ici_htonll(long long v);
void archive_byteswap(void *ptr, int sz);

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * An archiving session.
 */
class archiver
{
public:
    archiver(file *, objwsup *);
    virtual ~archiver();
    operator bool() const { return a_sent != nullptr; }
    inline objwsup *scope() const { return a_scope; }

    int record(object *, object *);
    object *lookup(object *);
    void remove(object *);

    virtual int read(void *buf, int len);
    virtual int write(const void *, int);

    inline int read(char *abyte) {
        return read(abyte, 1);
    }
    int read(int16_t *hword);
    int read(int32_t *aword);
    int read(int64_t *dword);
    int read(double *dbl);

    int write(unsigned char abyte) {
        return write(&abyte, 1);
    }
    int write(int16_t hword);
    int write(int32_t aword);
    int write(int64_t dword);
    int write(double adbl);

    int get() {
        char c;
        if (read(&c) == 1) return c;
        return -1;
    }

private:
    file *  a_file;
    map *   a_sent;
    objwsup *a_scope;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

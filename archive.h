// -*- mode:c++ -*-

/*
 * ici object serialization
 *
 * Copyright (C) 1995 A. Newman
 */

#ifndef ICI_ARCHIVE_H
#define ICI_ARCHIVE_H

#include "object.h"

namespace ici
{

int archive_init();
void archive_uninit();
int init_restorer_map();
void uninit_restorer_map();
int init_saver_map();
void uninit_saver_map();

int archive_f_save(...);
int archive_f_restore(...);

int archive_op_func_code(int (*fn)());
int (*archive_op_func(int))();

#if defined(__i386__) || defined(__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * The bit of the tcode set when the object is atomic.
 */
constexpr int O_ARCHIVE_ATOMIC = 0x80;

void archive_byteswap(void *ptr, int sz);

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * An archiving session.
 */
struct archive : object
{
    file *      a_file;  // The file used for saving or restoring.
    ici_struct *a_sent;  // Records archived object identity - int object address -> object
    objwsup *   a_scope; // The scope at the time of archiving

    static archive *start(ici_file_t *file, ici_objwsup_t *scope);

    int insert(object *key, object *val);
    void uninsert(object *key);
    object *lookup(object *obj);
    void stop();
};

inline ici_archive_t *archive_of(object *o) { return (ici_archive_t *)(o); }

/*
 * End of ici.h export. --ici.h-end--
 */

class archive_type : public type
{
public:
    archive_type() : type("archive", sizeof (struct archive)) {}
    unsigned long mark(object *o) override;
};

} // namespace ici

#endif

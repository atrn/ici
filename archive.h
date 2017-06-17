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
int ici_init_restorer_map();
void ici_uninit_restorer_map();
int ici_init_saver_map();
void ici_uninit_saver_map();
int archive_f_save(...);
int archive_f_restore(...);
int ici_archive_op_func_code(int (*fn)());
int (*ici_archive_op_func(int))();

#if defined(__i386__) || defined(__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * The bit of the tcode set when the object is atomic.
 */
constexpr int ICI_ARCHIVE_ATOMIC = 0x80;

void archive_byteswap(void *ptr, int sz);

/*
 * An archiving session.
 */
struct archive : object
{
    /* The file used for saving or restoring */
    ici_file_t *        a_file;
    /* Remembers which objects have been sent - maps object address as ints to object */
    ici_struct_t *      a_sent;
    /* The scope at the time of archive creation */
    ici_objwsup_t *     a_scope;
};

inline static ici_archive_t *archive_of(object *o) { return (ici_archive_t *)(o); }

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

ici_archive_t   *ici_archive_start(ici_file_t *file, ici_objwsup_t *scope);
int             ici_archive_insert(ici_archive_t *ar, object *key, object *val);
void            ici_archive_uninsert(ici_archive_t *ar, object *key);
object       *ici_archive_lookup(ici_archive_t *ar, object *obj);
void            ici_archive_stop(ici_archive_t *ar);

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

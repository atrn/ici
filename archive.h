// -*- mode:c++ -*-

/*
 * ici object serialization
 *
 * Copyright (C) 1995 A. Newman
 */

#ifndef ICI_ARCHIVE_H
#define ICI_ARCHIVE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

int ici_archive_init();
void ici_archive_uninit();
int ici_init_restorer_map();
void ici_uninit_restorer_map();
int ici_init_saver_map();
void ici_uninit_saver_map();
int ici_archive_f_save(...);
int ici_archive_f_restore(...);
int ici_archive_op_func_code(int (*fn)());
int (*ici_archive_op_func(int))();

#if defined(__i386__) || defined(__x86_64__)
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

/*
 * The bit of the tcode set when the object is atomic.
 */
#define ICI_ARCHIVE_ATOMIC 0x80

void ici_archive_byteswap(void *ptr, int sz);

/*
 * An archiving session.
 */
struct ici_archive : object
{
    /* The file used for saving or restoring */
    ici_file_t          *a_file;
    /* Used to remember which objects have been sent - maps object address, an int, to object */
    ici_struct_t        *a_sent;
    /* The scope at the time of archive creation */
    ici_objwsup_t	    *a_scope;
};

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
typedef struct ici_archive ici_archive_t;

ici_archive_t   *ici_archive_start(ici_file_t *file, ici_objwsup_t *scope);
int             ici_archive_insert(ici_archive_t *ar, ici_obj_t *key, ici_obj_t *val);
void            ici_archive_uninsert(ici_archive_t *ar, ici_obj_t *key);
ici_obj_t       *ici_archive_lookup(ici_archive_t *ar, ici_obj_t *obj);
void            ici_archive_stop(ici_archive_t *ar);

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

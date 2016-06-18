#ifndef ICI_MODULE_ARCHIVE_H
#define ICI_MODULE_ARCHIVE_H

#include <ici.h>

int ici_archive_f_save(void);
int ici_archive_f_restore(void);
int ici_archive_op_func_code(int (*fn)());
int (*ici_archive_op_func())(int code);

#ifdef __i386__
#define ICI_ARCHIVE_LITTLE_ENDIAN_HOST 1
#endif

#define ICI_ARCHIVE_ATOMIC 0x80

#define ICI_ARCHIVE_TCODE_REF (ICI_TC_MAX_CORE + 1)

void ici_archive_byteswap(void *ptr, int sz);

extern long ici_hash_string(ici_obj_t *);
extern void ici_pcre_info(pcre *, int *, void *);
extern ici_func_t *ici_new_func();
extern ici_src_t *ici_new_src(int, ici_str_t *);

/*
 * An archiving session
 */
struct ici_archive
{
    /* An archiving session is an ICI object */
    ici_obj_t           o_head;
    /* The file used for saving or restoring */
    ici_file_t          *a_file;
    /* Used to remember which objects have been sent */
    ici_struct_t        *a_sent;
    /* The scope at the time of archive creation */
    ici_objwsup_t	*a_scope;
};

typedef struct ici_archive ici_archive_t;

ici_archive_t   *ici_archive_start(ici_file_t *file, ici_objwsup_t *scope);
int             ici_archive_insert(ici_archive_t *ar, ici_obj_t *key, ici_obj_t *val);
void            ici_archive_uninsert(ici_archive_t *ar, ici_obj_t *key);
ici_obj_t       *ici_archive_lookup(ici_archive_t *ar, ici_obj_t *obj);
void            ici_archive_stop(ici_archive_t *ar);

#endif

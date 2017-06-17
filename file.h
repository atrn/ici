// -*- mode:c++ -*-

#ifndef ICI_FILE_H
#define ICI_FILE_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct file : object
{
    void        *f_file;
    ici_ftype_t *f_type;
    ici_str_t   *f_name;    /* Reasonable name to call it by. */
    ici_obj_t   *f_ref;
};
/*
 * f_ref                An object for this file object to reference.
 *                      This is used to reference the string when we
 *                      are treating a string as a file, and other cases,
 *                      to keep the object referenced. Basically if f_file
 *                      is an implicit reference to some object. May be NULL.
 */

inline ici_file_t *ici_fileof(ici_obj_t *o) { return static_cast<ici_file_t *>(o); }
inline bool ici_isfile(ici_obj_t *o) { return o->isa(ICI_TC_FILE); }

constexpr int ICI_F_CLOSED = 0x20;    /* File is closed. */
constexpr int ICI_F_NOCLOSE = 0x40;    /* Don't close on object free. */

/*
 * End of ici.h export. --ici.h-end--
 */

class file_type : public type
{
public:
    file_type() : type("file", sizeof (struct file)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

} // namespace ici

#endif /* ICI_FILE_H */

// -*- mode:c++ -*-

#ifndef ICI_FILE_H
#define ICI_FILE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * A set of function pointers for simple file abstraction.  ICI file objects
 * are implemented on top of this simple file abstraction in order to allow
 * several different types of file-like entities.  Each different type of file
 * uses one of these structures with specific functions.  Each function is
 * assumed to be compatible with the stdio function of the same name.  In the
 * case were the file is a stdio stream, these *are* the stdio functions.
 *
 * See also: 'ici_stdio_ftype'.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ftype
{
    int         ft_flags;
    int         (*ft_getch)(void *);
    int         (*ft_ungetch)(int, void *);
    int         (*ft_flush)(void *);
    int         (*ft_close)(void *);
    long        (*ft_seek)(void *, long, int);
    int         (*ft_eof)(void *);
    int         (*ft_write)(const void *, long, void *);
};
/*
 * ft_flags             A combination of FT_* flags, defined below.
 */

/*
 * The flags that may appear in ft_flags.  NOTE: If any flag greater than 0x80
 * is specified, file creation with ici_file_new() will fail.  See that
 * function for details.
 *
 * FT_NOMUTEX           ICI will surround file I/O on this file with
 *                      ici_leave()/ici_enter().  Using this flag can increase
 *                      efficiency by allowing other threads to run during a
 *                      blocking I/O operation; however it must NOT be used if
 *                      the file's I/O functions can access ICI data.
 *
 * --ici-api-- continued.
 */
constexpr int FT_NOMUTEX = 0x01;

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

} // namespace ici

#endif /* ICI_FILE_H */

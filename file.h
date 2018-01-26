// -*- mode:c++ -*-

#ifndef ICI_FILE_H
#define ICI_FILE_H

#include "object.h"
#include "ftype.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct file : object
{
    void   *f_file;
    ftype  *f_type;
    str    *f_name;    /* Reasonable name to call it by. */
    object *f_ref;

    inline int flags() const { return f_type->flags; }
    inline int getch() { return f_type->getch(f_file); }
    inline int ungetch(int c) { return f_type->ungetch(c, f_file); }
    inline int flush() { return f_type->flush(f_file); }
    inline int close() { return f_type->close(f_file); }
    inline long seek(long o, int w) { return f_type->seek(f_file, o, w); }
    inline int eof() { return f_type->eof(f_file); }
    inline int read(void *p, long n) { return f_type->read(p, n, f_file); }
    inline int write(const void *p, long n) { return f_type->write(p, n, f_file); }
    inline int fileno() { return f_type->fileno(f_file); }
    inline int setvbuf(char *p, int t, size_t z) { return f_type->setvbuf(f_file, p, t, z); }

    static constexpr int closed = 0x20;    /* File is closed. */
    static constexpr int noclose = 0x40;    /* Don't close on object free. */
};
/*
 * f_ref                An object for this file object to reference.
 *                      This is used to reference the string when we
 *                      are treating a string as a file, and other cases,
 *                      to keep the object referenced. Basically if f_file
 *                      is an implicit reference to some object. May be nullptr.
 */

inline file *fileof(object *o) { return o->as<file>(); }
inline bool isfile(object *o) { return o->isa(TC_FILE); }

/*
 * End of ici.h export. --ici.h-end--
 */

class file_type : public type
{
public:
    file_type() : type("file", sizeof (struct file)) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *fetch(object *o, object *k) override;
};

} // namespace ici

#endif /* ICI_FILE_H */

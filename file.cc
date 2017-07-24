#define ICI_CORE
#include "fwd.h"
#include "file.h"
#include "str.h"
#include "null.h"
#include "parse.h"
#include "primes.h"
#include "buf.h"
#include "int.h"
#include "null.h"

namespace ici
{


/*
 * Return a file object with the given 'ftype' and a file type specific
 * pointer 'fp' which is often something like a 'STREAM *' or a file
 * descriptor.  The 'name' is mostly for error messages and stuff.  The
 * returned object has a ref count of 1.  Returns NULL on error.
 *
 * The 'ftype' is a pointer to a struct of stdio-like function pointers that
 * will be used to do I/O operations on the file (see 'ftype').  The
 * given structure is assumed to exist as long as necessary.  (It is normally
 * a static srtucture, so this is not a problem.) The core-supplied struct
 * 'stdio_ftype' can be used if 'fp' is a 'STREAM *'.
 *
 * The 'ref' argument is an object reference that the file object will keep in
 * case the 'fp' argument is an implicit reference into some object (for
 * example, this is used for reading an ICI string as a file).  It may be NULL
 * if not required.
 *
 * This --func-- forms part of the --ici-api--.
 */
file *new_file(void *fp, ftype *ftype, str *name, object *ref)
{
    file *f;

    if ((f = ici_talloc(file)) == NULL)
        return NULL;
    set_tfnz(f, TC_FILE, 0, 1, 0);
    f->f_file = fp;
    f->f_type = ftype;
    f->f_name = name;
    f->f_ref = ref;
    rego(f);
    return f;
}

/*
 * Close the given ICI file 'f' by calling the lower-level close function
 * given in the 'ftype' associated with the file.  A guard flag is
 * maintained in the file object to prevent multiple calls to the lower level
 * function (this is really so we can optionally close the file explicitly,
 * and let the garbage collector do it to).  Returns non-zero on error, usual
 * conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int close_file(file *f)
{
    exec *x = NULL;
    int   r;

    if (f->flagged(file::closed))
    {
        return set_error("file already closed");
    }
    f->set(file::closed);
    if (f->flagged(ftype::nomutex))
        x = leave();
    r = f->close();
    if (f->flagged(ftype::nomutex))
        enter(x);
    /*
     * If this is a pipe opened with popen(), 'r' is actually the exit status
     * of the process.  If this is non-zero, format it into an error message.
     * Note: we can't do this within popen_ftype's close(), because
     * modifying error between calls to leave()/enter() is not
     * allowed.
     */
    if (r != 0 && f->f_type == popen_ftype)
    {
        set_error("popen command exit status %d", r);
    }
    return r;
}

size_t file_type::mark(object *o)
{
    auto f = fileof(o);
    return type::mark(f) + mark_optional(f->f_name) + mark_optional(f->f_ref);
}

void file_type::free(object *o)
{
    if (!o->flagged(file::closed)) {
        if (o->flagged(file::noclose)) {
            fileof(o)->flush();
	} else {
            close_file(fileof(o));
	}
    }
    ici_tfree(o, file);
}

int file_type::cmp(object *o1, object *o2)
{
    return fileof(o1)->f_file != fileof(o2)->f_file
	   || fileof(o1)->f_type != fileof(o2)->f_type;
}

object * file_type::fetch(object *o, object *k)
{
    if (k == SS(name)) {
        if (fileof(o)->f_name != NULL) {
            return fileof(o)->f_name;
	}
        return ici_null;
    }
    if (fileof(o)->f_type == parse_ftype && k == SS(line))
    {
        ici_int *l;

        if ((l = new_int(parseof(fileof(o)->f_file)->p_lineno)) != NULL) {
            l->decref();
	}
        return l;
    }
    return fetch_fail(o, k);
}

} // namespace ici

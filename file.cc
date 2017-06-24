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
ici_file_t *
ici_file_new(void *fp, ftype *ftype, str *name, object *ref)
{
    ici_file_t *f;

    if ((f = ici_talloc(ici_file_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(f, ICI_TC_FILE, 0, 1, 0);
    f->f_file = fp;
    f->f_type = ftype;
    f->f_name = name;
    f->f_ref = ref;
    ici_rego(f);
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
int
ici_file_close(ici_file_t *f)
{
    ici_exec_t  *x = NULL;
    int         r;

    if (f->flagged(ICI_F_CLOSED))
    {
        return ici_set_error("file already closed");
    }
    f->setflag(ICI_F_CLOSED);
    if (f->flags() & FT_NOMUTEX)
        x = ici_leave();
    r = f->close();
    if (f->flags() & FT_NOMUTEX)
        ici_enter(x);
    /*
     * If this is a pipe opened with popen(), 'r' is actually the exit status
     * of the process.  If this is non-zero, format it into an error message.
     * Note: we can't do this within popen_ftype's close(), because
     * modifying ici_error between calls to ici_leave()/ici_enter() is not
     * allowed.
     */
    if (r != 0 && f->f_type == popen_ftype)
    {
        ici_set_error("popen command exit status %d", r);
    }
    return r;
}

size_t file_type::mark(object *o)
{
    auto f = fileof(o);
    return setmark(f) + maybe_mark(f->f_name) + maybe_mark(f->f_ref);
}

void file_type::free(object *o)
{
    if (!o->flagged(ICI_F_CLOSED))
    {
        if (o->flagged(ICI_F_NOCLOSE))
            fileof(o)->flush();
        else
            ici_file_close(fileof(o));
    }
    ici_tfree(o, ici_file_t);
}

int file_type::cmp(object *o1, object *o2)
{
    return fileof(o1)->f_file != fileof(o2)->f_file
    || fileof(o1)->f_type != fileof(o2)->f_type;
}

object * file_type::fetch(object *o, object *k)
{
    if (k == SS(name))
    {
        if (fileof(o)->f_name != NULL)
            return fileof(o)->f_name;
        return ici_null;
    }
    if (fileof(o)->f_type == parse_ftype && k == SS(line))
    {
        ici_int_t   *l;

        if ((l = ici_int_new(ici_parseof(fileof(o)->f_file)->p_lineno)) != NULL)
            l->decref();
        return l;
    }
    return fetch_fail(o, k);
}

} // namespace ici

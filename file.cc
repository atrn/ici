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
#include "types.h"

namespace ici
{


/*
 * Return a file object with the given 'ftype' and a file type specific
 * pointer 'fp' which is often something like a 'STREAM *' or a file
 * descriptor.  The 'name' is mostly for error messages and stuff.  The
 * returned object has a ref count of 1.  Returns NULL on error.
 *
 * The 'ftype' is a pointer to a struct of stdio-like function pointers that
 * will be used to do I/O operations on the file (see 'ici_ftype_t').  The
 * given structure is assumed to exist as long as necessary.  (It is normally
 * a static srtucture, so this is not a problem.) The core-supplied struct
 * 'ici_stdio_ftype' can be used if 'fp' is a 'STREAM *'.
 *
 * The 'ref' argument is an object reference that the file object will keep in
 * case the 'fp' argument is an implicit reference into some object (for
 * example, this is used for reading an ICI string as a file).  It may be NULL
 * if not required.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_file_t *
ici_file_new(void *fp, ici_ftype_t *ftype, ici_str_t *name, ici_obj_t *ref)
{
    ici_file_t *f;

    if (ftype->ft_flags & ~0xFF)
    {
        /*
         * Old ici_ftype_t's have a function pointer rather than flags in the
         * first field.  If any still exist in third-party extensions, this
         * should trap them.
         */
        ici_set_error("old-style file type is no longer supported");
        return NULL;
    }
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
 * given in the 'ici_ftype_t' associated with the file.  A guard flag is
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

    if (f->o_flags & ICI_F_CLOSED)
    {
        return ici_set_error("file already closed");
    }
    f->o_flags |= ICI_F_CLOSED;
    if (f->f_type->ft_flags & FT_NOMUTEX)
        x = ici_leave();
    r = (*f->f_type->ft_close)(f->f_file);
    if (f->f_type->ft_flags & FT_NOMUTEX)
        ici_enter(x);
    /*
     * If this is a pipe opened with popen(), 'r' is actually the exit status
     * of the process.  If this is non-zero, format it into an error message.
     * Note: we can't do this within ici_popen_ftype's ft_close(), because
     * modifying ici_error between calls to ici_leave()/ici_enter() is not
     * allowed.
     */
    if (r != 0 && f->f_type == &ici_popen_ftype)
    {
        ici_set_error("popen command exit status %d", r);
    }
    return r;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */

unsigned long file_type::mark(ici_obj_t *o)
{
    long        mem;

    o->o_flags |= ICI_O_MARK;
    mem = sizeof(ici_file_t);
    if (ici_fileof(o)->f_name != NULL)
        mem += ici_mark(ici_fileof(o)->f_name);
    if (ici_fileof(o)->f_ref != NULL)
        mem += ici_mark(ici_fileof(o)->f_ref);
    return mem;
}

void file_type::free(ici_obj_t *o)
{
    if ((o->o_flags & ICI_F_CLOSED) == 0)
    {
        if (o->o_flags & ICI_F_NOCLOSE)
            (*ici_fileof(o)->f_type->ft_flush)(ici_fileof(o)->f_file);
        else
            ici_file_close(ici_fileof(o));
    }
    ici_tfree(o, ici_file_t);
}

int file_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    return ici_fileof(o1)->f_file != ici_fileof(o2)->f_file
    || ici_fileof(o1)->f_type != ici_fileof(o2)->f_type;
}

ici_obj_t * file_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    if (k == SSO(name))
    {
        if (ici_fileof(o)->f_name != NULL)
            return ici_fileof(o)->f_name;
        return ici_null;
    }
    if (ici_fileof(o)->f_type == &ici_parse_ftype && k == SSO(line))
    {
        ici_int_t   *l;

        if ((l = ici_int_new(ici_parseof(ici_fileof(o)->f_file)->p_lineno)) != NULL)
            l->decref();
        return l;
    }
    return ici_fetch_fail(o, k);
}

} // namespace ici

#define ICI_CORE
#include "file.h"
#include "archiver.h"
#include "buf.h"
#include "fwd.h"
#include "int.h"
#include "null.h"
#include "parse.h"
#include "primes.h"
#include "str.h"

namespace ici
{

/*
 * Return a file object with the given 'ftype' and a file type specific
 * pointer 'fp' which is often something like a 'STREAM *' or a file
 * descriptor.  The 'name' is mostly for error messages and stuff.  The
 * returned object has a ref count of 1.  Returns nullptr on error.
 *
 * The 'ftype' is a pointer to a struct of stdio-like function pointers that
 * will be used to do I/O operations on the file (see 'ftype').  The
 * given structure is assumed to exist as long as necessary.  (It is normally
 * a static srtucture, so this is not a problem.) The core-supplied struct
 * 'stdio_ftype' can be used if 'fp' is a 'STREAM *'.
 *
 * The 'ref' argument is an object reference that the file object will keep in
 * case the 'fp' argument is an implicit reference into some object (for
 * example, this is used for reading an ICI string as a file).  It may be nullptr
 * if not required.
 *
 * This --func-- forms part of the --ici-api--.
 */
file *new_file(void *fp, ftype *ftype, str *name, object *ref)
{
    file *f;

    if ((f = ici_talloc(file)) == nullptr)
    {
        return nullptr;
    }
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
    exec *x = nullptr;
    int   r;

    if (f->hasflag(file::closed))
    {
        return set_error("file already closed");
    }
    f->set(file::closed);
    if (f->hasflag(ftype::nomutex))
    {
        x = leave();
    }
    r = f->close();
    if (f->hasflag(ftype::nomutex))
    {
        enter(x);
    }
    /*
     * If this is a pipe opened with popen(), 'r' is actually the exit status
     * of the process.  If this is non-zero, format it into an error message.
     * Note: we can't do this within popen_ftype's close(), because
     * modifying error between calls to leave()/enter() is not
     * allowed.
     */
    if (r != 0 && f->f_type == popen_ftype)
    {
        set_error("exit status %d", r);
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
    if (!o->hasflag(file::closed))
    {
        if (o->hasflag(file::noclose))
        {
            fileof(o)->flush();
        }
        else
        {
            close_file(fileof(o));
        }
    }
    ici_tfree(o, file);
}

int file_type::cmp(object *o1, object *o2)
{
    return fileof(o1)->f_file != fileof(o2)->f_file || fileof(o1)->f_type != fileof(o2)->f_type;
}

object *file_type::fetch(object *o, object *k)
{
    if (k == SS(name))
    {
        if (fileof(o)->f_name != nullptr)
        {
            return fileof(o)->f_name;
        }
        return null;
    }
    if (fileof(o)->f_type == parse_ftype && k == SS(line))
    {
        integer *l;

        if ((l = new_int(parseof(fileof(o)->f_file)->p_lineno)) != nullptr)
        {
            decref(l);
        }
        return l;
    }
    return fetch_fail(o, k);
}

int file_type::save(archiver *ar, object *o)
{
    if (fileof(o)->f_type == stdio_ftype)
    {
        FILE *f = (FILE *)fileof(o)->f_file;
        if (f == stdin)
        {
            return ar->write(uint8_t('i'));
        }
        if (f == stdout)
        {
            return ar->write(uint8_t('o'));
        }
        if (f == stderr)
        {
            return ar->write(uint8_t('e'));
        }
    }
    return type::save(ar, o);
}

object *file_type::restore(archiver *ar)
{
    uint8_t code;
    if (ar->read(&code))
    {
        return nullptr;
    }
    if (code == 'i')
    {
        return need_stdin();
    }
    if (code == 'o')
    {
        return need_stdout();
    }
    if (code == 'e')
    {
        return need_stderr();
    }
    set_error("unexpected stream code (%u) when restoring file", code);
    return nullptr;
}

} // namespace ici

#define ICI_CORE

#include "buf.h"
#include "file.h"
#include "ftype.h"
#include "fwd.h"
#include "mem.h"
#include "str.h"

namespace ici
{

/*
 * This structure is used wherever a character buffer in memory is treated as
 * an ICI file object.  Two related file types reference this structure:
 *
 * charbuf_ftype:   used for atomic string objects, memory objects, and
 *                  simple C strings, which cannot move in memory.
 *
 * strbuf_ftype:    used for mutable string buffer objects, which can move
 *                  unexpectedly in memory.
 */
struct charbuf
{
    char   *cb_data;
    char   *cb_ptr;
    int     cb_size;
    int     cb_eof;
    object *cb_ref;
    int     cb_readonly;
};

class charbuf_ftype : public ftype
{
  public:
    int getch(void *file) override
    {
        charbuf *cb = (charbuf *)file;
        if (cb->cb_ptr < cb->cb_data || cb->cb_ptr >= (cb->cb_data + cb->cb_size))
        {
            cb->cb_eof = 1;
            return EOF;
        }
        cb->cb_eof = 0;
        return *cb->cb_ptr++ & 0xFF;
    }

    int read(void *buf, long n, void *file) override
    {
        charbuf *cb = (charbuf *)file;
        int      r = cb->cb_ptr - cb->cb_data;
        int      a = cb->cb_size - r;
        int      m = a < n ? a : n;
        memcpy(buf, cb->cb_ptr, m);
        return int(m);
    }

    int ungetch(int c, void *file) override
    {
        charbuf *cb = (charbuf *)file;
        if (c == EOF || cb->cb_ptr <= cb->cb_data || cb->cb_ptr > (cb->cb_data + cb->cb_size))
        {
            return EOF;
        }
        *--cb->cb_ptr = c;
        cb->cb_eof = 0;
        return c;
    }

    int close(void *file) override
    {
        charbuf *cb = (charbuf *)file;
        if (cb->cb_ref == nullptr)
        {
            ici_free(cb->cb_data);
        }
        ici_tfree(cb, charbuf);
        return 0;
    }

    long seek(void *file, long offset, int whence) override
    {
        charbuf *cb = (charbuf *)file;
        switch (whence)
        {
        case 0:
            if (offset < 0 || offset >= cb->cb_size)
            {
                return -1;
            }
            cb->cb_ptr = cb->cb_data + offset;
            break;

        case 1:
            if (offset < 0)
            {
                const auto lim = -(cb->cb_ptr - cb->cb_data);
                if (offset < lim)
                {
                    return -1;
                }
            }
            else if (offset > 0)
            {
                const auto lim = cb->cb_size - (cb->cb_ptr - cb->cb_data);
                if (offset >= lim)
                {
                    return -1;
                }
            }
            cb->cb_ptr += offset;
            break;

        case 2:
            if (offset > 0)
            {
                return -1;
            }
            cb->cb_ptr = cb->cb_data + cb->cb_size + offset;
            break;
        }
        return (long)(cb->cb_ptr - cb->cb_data);
    }

    int eof(void *file) override
    {
        charbuf *cb = (charbuf *)file;
        return cb->cb_eof;
    }

    int write(const void *data, long count, void *file) override
    {
        charbuf *cb = (charbuf *)file;
        if (cb->cb_readonly || count <= 0)
        {
            return 0;
        }
        if (cb->cb_ptr < cb->cb_data || cb->cb_ptr >= cb->cb_data + cb->cb_size)
        {
            return 0;
        }
        if (count > cb->cb_data + cb->cb_size - cb->cb_ptr)
        {
            count = cb->cb_data + cb->cb_size - cb->cb_ptr;
        }
        memcpy(cb->cb_ptr, data, count);
        cb->cb_ptr += count;
        return count;
    }
};

/*
 * charbuf_ftype is used for buffers which cannot move in memory.  This
 * includes atomic string objects, memory objects, and C strings.  It is
 * slightly more efficient at reading and writing than strbuf_ftype is,
 * since it stores pointers directly into the immovable buffer.
 *
 * This type replaces string_ftype (which was a misnomer since it could be
 * used for memory objects but not mutable string objects).
 */
ftype *charbuf_ftype = instanceof <class charbuf_ftype>();

static void reattach_string_buffer(charbuf *sb)
{
    int index = sb->cb_ptr - sb->cb_data;
    sb->cb_data = stringof(sb->cb_ref)->s_chars;
    sb->cb_size = stringof(sb->cb_ref)->s_nchars;
    sb->cb_ptr = sb->cb_data + index;
#if ICI_KEEP_STRING_HASH
    {
        str *s = stringof(sb->cb_ref);
        s->s_hash = 0;
        hash_string(s);
    }
#endif
}

class stringbuf_ftype : public charbuf_ftype
{
  public:
    int getch(void *file) override
    {
        charbuf *sb = (charbuf *)file;
        reattach_string_buffer(sb);
        return charbuf_ftype::getch(sb);
    }

    int ungetch(int c, void *file) override
    {
        charbuf *sb = (charbuf *)file;
        reattach_string_buffer(sb);
        return charbuf_ftype::ungetch(c, sb);
    }

    long seek(void *file, long offset, int whence) override
    {
        charbuf *sb = (charbuf *)file;
        reattach_string_buffer(sb);
        return charbuf_ftype::seek(sb, offset, whence);
    }

    int write(const void *ptr, long count, void *file) override
    {
        const char *data = (const char *)ptr;
        charbuf    *sb = (charbuf *)file;
        str        *s;
        size_t      size;

        if (sb->cb_readonly || count <= 0)
        {
            return 0;
        }
        if (sb->cb_ptr < sb->cb_data || sb->cb_ptr > sb->cb_data + sb->cb_size)
        {
            return 0;
        }
        s = stringof(sb->cb_ref);
        size = sb->cb_ptr - sb->cb_data + count;
        if (str_need_size(s, size))
        {
            return 0;
        }
        if (s->s_nchars < size)
        {
            s->s_nchars = size;
        }
        s->s_chars[s->s_nchars] = '\0';
        reattach_string_buffer(sb);
        memcpy(sb->cb_ptr, data, count);
        sb->cb_ptr += count;
        return count;
    }
};

/*
 * strbuf_ftype is used for mutable string buffer objects, which can move
 * unexpectedly in memory as they grow.  Because of this, this file type must
 * check on every file operation whether the referenced string buffer has
 * moved.  This makes it less efficient than charbuf_ftype for buffers
 * that are known to be immovable.
 */
ftype *strbuf_ftype = instanceof <class stringbuf_ftype>();

/*
 * Create an ICI file object that treats the character buffer referenced by
 * 'data' (of length 'size') as a file.  If 'ref' is non-nullptr it must refer to
 * either a string object or a memory object that owns the data, and the data
 * is used in-place.  But if 'ref' is nullptr, it is assumed that the data must
 * be copied into a private allocation first.  The private allocation will be
 * freed when the file is closed.
 *
 * If 'readonly' is non-zero, the returned file gives read-only access to the
 * data.  If 'readonly' is zero, 'ref' must refer to either a mutable string
 * buffer object (created by new_str_buf()) or a memory object, and the
 * file gives read/write access to the object's data.  The object's initial
 * data is preserved.  Writing past the end of a string buffer will extend it;
 * writing past the end of a memory object gets truncated.
 *
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
file *open_charbuf(char *data, int size, object *ref, bool readonly)
{
    file    *f = nullptr;
    charbuf *cb;

    if ((cb = ici_talloc(charbuf)) == nullptr)
    {
        return nullptr;
    }
    cb->cb_size = size;
    cb->cb_eof = 0;
    cb->cb_readonly = readonly;
    cb->cb_ref = ref;
    if (ref != nullptr)
    {
        /*
         * Use this object's data in-place; the ici_file_t object keeps a
         * reference to it so it won't go away.
         */
        cb->cb_data = data;
        cb->cb_ptr = data;
        /*
         * If this is a mutable string buffer, use strbuf_ftype.  If this
         * is an atomic (immutable) string, or a memory object, it is slightly
         * more efficient to use charbuf_ftype.  Of course an atomic
         * string can't be opened for writing.
         */
        if (isstring(ref))
        {
            if (ref->flags(object::O_ATOM | ICI_S_SEP_ALLOC) == ICI_S_SEP_ALLOC)
            {
                f = new_file((char *)cb, strbuf_ftype, nullptr, ref);
            }
            else if (readonly)
            {
                f = new_file((char *)cb, charbuf_ftype, nullptr, ref);
            }
            else
            {
                set_error("attempt to open an atomic string for writing");
            }
        }
        else if (ismem(ref))
        {
            f = new_file((char *)cb, charbuf_ftype, nullptr, ref);
        }
        else if (!chkbuf(50))
        {
            char n[objnamez];
            set_error("attempt to open %s as a char buffer", objname(n, ref));
        }
    }
    else
    {
        /*
         * This is not an ici object, so it's read only, and we need to take a
         * copy of it.
         */
        if (readonly)
        {
            if ((cb->cb_data = (char *)ici_alloc(size)) != nullptr)
            {
                memcpy(cb->cb_data, data, size);
                cb->cb_ptr = cb->cb_data;
                f = new_file((char *)cb, charbuf_ftype, nullptr, ref);
                if (f == nullptr)
                {
                    ici_free(cb->cb_data);
                }
            }
        }
        else
        {
            set_error("attempt to open non-object char buffer for writing");
        }
    }
    if (f == nullptr)
    {
        ici_tfree(cb, charbuf);
    }
    return f;
}

} // namespace ici

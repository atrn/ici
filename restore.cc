#define ICI_CORE

/*
 * any = restore([file] [, struct])
 *
 * Read and return an object from a file using the ICI serialized object protocol.
 *
 * If file is not given the object is read from the standard input.
 *
 * If the struct is given it is used as the static scope for any function
 * objects restored.
 *
 * This --topic-- forms part of the --ici-serialisation-- documentation.
 */

#include "fwd.h"
#include "restorer.h"
#include "archive.h"
#include "int.h"
#include "float.h"
#include "array.h"
#include "struct.h"
#include "set.h"
#include "src.h"
#include "file.h"
#include "str.h"
#include "func.h"
#include "null.h"
#include "re.h"
#include "set.h"
#include "mem.h"
#include "ptr.h"
#include "op.h"
#include "mark.h"

#include <assert.h>

#include <netinet/in.h>

namespace ici
{

static object *restore(archive *);

static int readf(archive *ar, void *buf, int len)
{
    char *p = (char *)buf;
    while (len-- > 0)
    {
        int ch;

        if ((ch = ar->get()) == -1)
        {
            ici_set_error("eof");
	    return 1;
        }
	*p++ = ch;
    }
    return 0;
}

inline int read8(archive *ar, char *abyte)
{
    return readf(ar, abyte, 1);
}

static int read16(archive *ar, int16_t *hword)
{
    int16_t tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
    	return 1;
    }
    *hword = ntohs(tmp);
    return 0;
}

static int
read32(archive *ar, int32_t *aword)
{
    int32_t tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
    	return 1;
    }
    *aword = ntohl(tmp);
    return 0;
}

static int
read64(archive *ar, int64_t *dword)
{
    int64_t tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
    	return 1;
    }
    *dword = ntohll(tmp);
    return 0;
}

static int
readdbl(archive *ar, double *dbl)
{
    return readf(ar, dbl, sizeof *dbl);
}

static int
restore_obj(archive *ar, char *flags)
{
    char tcode;

    if (read8(ar, &tcode))
    {
    	return -1;
    }
    *flags = tcode & O_ARCHIVE_ATOMIC ? ICI_O_ATOM : 0;
    tcode &= ~O_ARCHIVE_ATOMIC;
    return tcode;
}

inline int restore_object_name(archive *ar, object **name)
{
    return readf(ar, name, sizeof *name);
}

static object *
restore_error(archive *)
{
    ici_set_error("unable to restore object");
    return NULL;
}

// null

static object *
restore_null(archive *)
{
    return ici_null;
}

// int

static object *
restore_int(archive *ar)
{
    int64_t value;

    if (read64(ar, &value))
    {
        return NULL;
    }
    return ici_int_new(value);
}

// float

static object *
restore_float(archive *ar)
{
    double val;

    if (readdbl(ar, &val))
    {
        return NULL;
    }
#if ICI_ARCHIVE_LITTLE_ENDIAN_HOST
    archive_byteswap(&val, sizeof val);
#endif
    return ici_float_new(val);
}

// string

static object *
restore_string(archive *ar)
{
    str *s;
    int32_t len;
    object *name;
    object *obj;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (read32(ar, &len))
    {
        return NULL;
    }
    if ((s = ici_str_alloc(len)) == NULL)
    {
        return NULL;
    }
    if (readf(ar, s->s_chars, len))
    {
        goto fail;
    }
    ici_hash_string(s);
    if ((obj = ici_atom(s, 1)) == NULL)
    {
        goto fail;
    }
    if (ar->insert(name, obj))
    {
        obj->decref();
        goto fail;
    }
    return obj;

fail:
    s->decref();
    return NULL;
}

// regexp

static object *
restore_regexp(archive *ar)
{
    object *r;
    int options;
    str *s;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (read32(ar, &options))
    {
        return NULL;
    }
    if ((s = ici_stringof(restore_string(ar))) == NULL)
    {
        return NULL;
    }
    r = ici_regexp_new(s, options);
    if (r == NULL)
    {
        s->decref();
        return NULL;
    }
    s->decref();
    if (ar->insert(name, r))
    {
        r->decref();
        return NULL;
    }
    return r;
}

// mem

static object *
restore_mem(archive *ar)
{
    int64_t len;
    int16_t accessz;
    size_t sz;
    void *p;
    mem *m = 0;
    object *name;

    if (restore_object_name(ar, &name) || read64(ar, &len) || read16(ar, &accessz))
    {
        return NULL;
    }
    sz = size_t(len) * size_t(accessz);
    if ((p = ici_alloc(sz)) != NULL)
    {
        if ((m = ici_mem_new(p, len, accessz, ici_free)) == NULL)
        {
            ici_free(p);
        }
        else if (readf(ar, p, sz) || ar->insert(name, m))
        {
            m->decref();
            m = NULL;
        }
    }
    return m;
}

// array

static object *
restore_array(archive *ar)
{
    int64_t n;
    array *a;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (read64(ar, &n))
    {
        return NULL;
    }
    if ((a = ici_array_new(n)) == NULL)
    {
        return NULL;
    }
    if (ar->insert(name, a))
    {
        goto fail;
    }
    for (; n > 0; --n)
    {
        object *o;

        if ((o = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if (a->push(o))
        {
            o->decref();
            goto fail1;
        }
	o->decref();
    }
    return a;

fail1:
    ar->uninsert(name);

fail:
    a->decref();
    return NULL;
}

// set

static object *
restore_set(archive *ar)
{
    set *s;
    int64_t n;
    int64_t i;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = ici_set_new()) == NULL)
    {
        return NULL;
    }
    if (ar->insert(name, s))
    {
        goto fail;
    }
    if (read64(ar, &n))
    {
        goto fail1;
    }
    for (i = 0; i < n; ++i)
    {
        object *o;

        if ((o = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if (ici_assign(s, o, ici_one))
        {
            o->decref();
            goto fail1;
        }
        o->decref();
    }
    return s;

fail1:
    ar->uninsert(name);

fail:
    s->decref();
    return NULL;
}

// struct

static object *
restore_struct(archive *ar)
{
    ici_struct *s;
    object *super;
    int64_t n;
    long i;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = ici_struct_new()) == NULL)
    {
        return NULL;
    }
    if (ar->insert(name, s))
    {
        goto fail;
    }
    if ((super = restore(ar)) == NULL)
    {
        goto fail1;
    }
    if (super != ici_null)
    {
        ici_objwsupof(s)->o_super = ici_objwsupof(super);
        super->decref();
    }

    if (read64(ar, &n))
    {
        goto fail1;
    }
    for (i = 0; i < n; ++i)
    {
        object *key;
        object *value;
        int failed;

        if ((key = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if ((value = restore(ar)) == NULL)
        {
            key->decref();
            goto fail1;
        }
        failed = ici_assign(s, key, value);
        key->decref();
        value->decref();
        if (failed)
        {
            goto fail1;
        }
    }
    return s;

fail1:
    ar->uninsert(name);

fail:
    s->decref();
    return NULL;
}

// ptr

static object *
restore_ptr(archive *ar)
{
    object *aggr;
    object *key;
    ptr *ptr;

    if ((aggr = restore(ar)) == NULL)
    {
        return NULL;
    }
    if ((key = restore(ar)) == NULL)
    {
        aggr->decref();
        return NULL;
    }
    ptr = ici_ptr_new(aggr, key);
    aggr->decref();
    key->decref();
    return ptr ? ptr : NULL;
}

// func

static object *
restore_func(archive *ar)
{
    object *code;
    object *args = NULL;
    object *autos = NULL;
    object *name = NULL;
    int32_t nautos;
    func *fn;
    object *oname;

    if (restore_object_name(ar, &oname))
    {
        return NULL;
    }
    if ((code = restore(ar)) == NULL)
    {
        return NULL;
    }
    if ((args = restore(ar)) == NULL)
    {
        goto fail;
    }
    if ((autos = restore(ar)) == NULL)
    {
        goto fail;
    }
    if ((name = restore(ar)) == NULL)
    {
        goto fail;
    }
    if (read32(ar, &nautos))
    {
        goto fail;
    }
    if ((fn = ici_new_func()) == NULL)
    {
        goto fail;
    }

    fn->f_code = arrayof(code);
    fn->f_args = arrayof(args);
    fn->f_autos = ici_structof(autos);
    fn->f_autos->o_super = ar->scope(); /* structof(ici_vs.a_top[-1])->o_super; */
    fn->f_name = ici_stringof(name);
    fn->f_nautos = nautos;

    code->decref();
    args->decref();
    autos->decref();
    name->decref();

    if (!ar->insert(oname, fn))
    {
	return fn;
    }

    /*FALLTHROUGH*/

fail:
    code->decref();
    if (args)
    {
        args->decref();
    }
    if (autos)
    {
        autos->decref();
    }
    if (name)
    {
        name->decref();
    }
    return NULL;
}

static object *
restore_op(archive *ar)
{
    int16_t op_func_code;
    int16_t op_ecode;
    int16_t op_code;

    if
    (
        read16(ar, &op_func_code)
        ||
        read16(ar, &op_ecode)
        ||
        read16(ar, &op_code)
    )
    {
        return 0;
    }

    return ici_new_op(archive_op_func(op_func_code), op_ecode, op_code);
}

static object *
restore_src(archive *ar)
{
    int32_t line;
    object *result;
    object *filename;

    if (read32(ar, &line))
    {
        return NULL;
    }
    if ((filename = restore(ar)) == NULL)
    {
        return NULL;
    }
    if (!ici_isstring(filename))
    {
        ici_set_error("unexpected type of filename");
        filename->decref();
        return NULL;
    }
    if ((result = new_src(line, ici_stringof(filename))) == NULL)
    {
        filename->decref();
        return NULL;
    }
    filename->decref();
    return result;
}

// cfunc

static object *
restore_cfunc(archive *ar)
{
    int16_t namelen;
    char space[32];
    char *buf;
    str *func_name;
    object *fn;

    if (read16(ar, &namelen))
    {
        return 0;
    }
    buf = space;
    if (namelen >= (int)(sizeof space))
    {
        if ((buf = (char *)ici_alloc(namelen)) == NULL)
        {
            return 0;
        }
    }
    if (readf(ar, buf, namelen))
    {
        ici_free(buf);
        return 0;
    }
    if ((func_name = ici_str_new(buf, namelen)) == NULL)
    {
        ici_free(buf);
        return 0;
    }
    if (buf != space)
    {
        ici_free(buf);
    }
    fn = ici_fetch(ar->scope(), func_name);
    func_name->decref();
    return fn;
}

static object *
restore_mark(archive *ar)
{
    return &ici_o_mark;
}

// ref

static object *
restore_ref(archive *ar)
{
    object *obj;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((obj = ar->lookup(name)) == ici_null)
    {
        obj = NULL;
    }
    return obj;
}

// restorer

static restorer_t *
restorer_new(object *(*fn)(archive *))
{
    restorer_t *r;

    if ((r = ici_talloc(restorer_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(r, ICI_TC_RESTORER, 0, 1, sizeof (restorer_t));
        r->r_fn = fn;
        ici_rego(r);
    }
    return r;
}

static ici_struct *restorer_map = 0;

static int
add_restorer(int tcode, object *(*fn)(archive *))
{
    restorer_t *r;
    ici_int *t = 0;

    if ((r = restorer_new(fn)) == NULL)
    {
        return 1;
    }
    if ((t = ici_int_new(tcode)) == NULL)
    {
        goto fail;
    }
    if (ici_assign(restorer_map, t, r))
    {
        t->decref();
        goto fail;
    }
    t->decref();
    r->decref();
    return 0;

fail:
    r->decref();
    return 1;
}

static restorer_t *
fetch_restorer(int key)
{
    object   *k;
    object   *v = NULL;

    if ((k = ici_int_new(key)) != NULL)
    {
        v = ici_fetch(restorer_map, k);
        k->decref();
    }
    return (restorer_t *)v;
}

void
uninit_restorer_map()
{
    restorer_map->decref();
}

int
init_restorer_map()
{
    size_t i;

    static struct
    {
        int tcode;
        object *(*fn)(archive *);
    }
    fns[] =
    {
        {-1,            restore_error},
        {ICI_TC_NULL,   restore_null},
        {ICI_TC_INT,    restore_int},
        {ICI_TC_FLOAT,  restore_float},
        {ICI_TC_STRING, restore_string},
        {ICI_TC_REGEXP, restore_regexp},
        {ICI_TC_MEM,    restore_mem},
        {ICI_TC_ARRAY,  restore_array},
        {ICI_TC_SET,    restore_set},
        {ICI_TC_STRUCT, restore_struct},
        {ICI_TC_PTR,    restore_ptr},
        {ICI_TC_FUNC,   restore_func},
        {ICI_TC_OP,     restore_op},
        {ICI_TC_SRC,    restore_src},
        {ICI_TC_CFUNC,  restore_cfunc},
        {ICI_TC_MARK,   restore_mark},
        {ICI_TC_REF,    restore_ref}
    };

    if ((restorer_map = ici_struct_new()) == NULL)
    {
        return 1;
    }
    for (i = 0; i < nels(fns); ++i)
    {
        if (add_restorer(fns[i].tcode, fns[i].fn))
        {
            restorer_map->decref();
            restorer_map = NULL;
            return 1;
        }
    }
    return 0;
}

static restorer_t *
get_restorer(int tcode)
{
    restorer_t  *r = fetch_restorer(tcode);
    if (isnull(r))
    {
        r = fetch_restorer(-1);
        if (isnull(r))
        {
            ici_set_error("archive module internal error");
            r = NULL;
        }
    }
    return r;
}

static object *
restore(archive *ar)
{
    object *obj = NULL;
    char flags;
    int tcode;

    if ((tcode = restore_obj(ar, &flags)) != -1)
    {
        restorer_t *restorer;

        if ((restorer = get_restorer(tcode)) != NULL && (obj = (*restorer->r_fn)(ar)) != NULL)
        {
            if (flags & ICI_O_ATOM)
            {
                obj = ici_atom(obj, 1);
            }
        }
    }
    return obj;
}

int
f_archive_restore(...)
{
    file *file;
    archive *ar;
    objwsup *scp;
    object *obj = NULL;

    scp = ici_structof(ici_vs.a_top[-1])->o_super;
    switch (NARGS())
    {
    case 0:
        if ((file = ici_need_stdin()) == NULL)
        {
            return 1;
        }
	break;

    case 1:
        if (typecheck("u", &file))
	{
            if (typecheck("d", &scp))
            {
		return 1;
            }
	    if ((file = ici_need_stdin()) == NULL)
            {
		return 1;
            }
	}
	break;

    default:
	if (typecheck("ud", &file, &scp))
        {
	    return 1;
        }
	break;
    }

    if ((ar = archive::start(file, scp)) != NULL)
    {
        obj = restore(ar);
        ar->stop();
    }

    return obj == NULL ? 1 : ici_ret_with_decref(obj);
}

} // namespace ici

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
#include "types.h"

#include <assert.h>

#include <netinet/in.h>

namespace ici
{

static ici_obj_t *restore(ici_archive_t *);

static int
readf(ici_archive_t *ar, void *buf, int len)
{
    char *p = (char *)buf;
    while (len-- > 0)
    {
        int (*get)(void *) = ar->a_file->f_type->ft_getch;
        int ch;

        if ((ch = (*get)(ar->a_file->f_file)) == -1)
        {
            ici_set_error("eof");
	    return 1;
        }
	*p++ = ch;
    }
    return 0;
}

inline
static int
read8(ici_archive_t *ar, char *abyte)
{
    return readf(ar, abyte, 1);
}

inline
static int
read16(ici_archive_t *ar, int16_t *hword)
{
    int16_t tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
    	return 1;
    }
    *hword = ntohs(tmp);
    return 0;
}

inline
static int
read32(ici_archive_t *ar, int32_t *aword)
{
    int32_t tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
    	return 1;
    }
    *aword = ntohl(tmp);
    return 0;
}

inline
static int
readl(ici_archive_t *ar, long *along)
{
    long tmp;
    if (readf(ar, &tmp, sizeof tmp))
    {
        return 1;
    }
    *along = ntohl(tmp);
    return 0;
}

inline
static int
readdbl(ici_archive_t *ar, double *dbl)
{
    return readf(ar, dbl, sizeof *dbl);
}

static int
restore_obj(ici_archive_t *ar, char *flags)
{
    char tcode;

    if (read8(ar, &tcode))
    {
    	return -1;
    }
    *flags = tcode & ICI_ARCHIVE_ATOMIC ? ICI_O_ATOM : 0;
    tcode &= ~ICI_ARCHIVE_ATOMIC;
    return tcode;
}

inline
static int
restore_object_name(ici_archive_t *ar, ici_obj_t **name)
{
    return readf(ar, name, sizeof *name);
}

static ici_obj_t *
restore_error(ici_archive_t *)
{
    ici_set_error("unable to restore object");
    return NULL;
}

// null

static ici_obj_t *
restore_null(ici_archive_t *)
{
    return ici_null;
}

// int

static ici_obj_t *
restore_int(ici_archive_t *ar)
{
    long        value;

    if (readl(ar, &value))
    {
        return NULL;
    }
    return ici_int_new(value);
}

// float

static ici_obj_t *
restore_float(ici_archive_t *ar)
{
    double val;

    if (readdbl(ar, &val))
    {
        return NULL;
    }
#if ICI_ARCHIVE_LITTLE_ENDIAN_HOST
    ici_archive_byteswap(&val, sizeof val);
#endif
    return ici_float_new(val);
}

// string

static ici_obj_t *
restore_string(ici_archive_t *ar)
{
    ici_str_t *s;
    long len;
    ici_obj_t *name;
    ici_obj_t *obj;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (readl(ar, &len))
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
    if (ici_archive_insert(ar, name, obj))
    {
        ici_decref(obj);
        goto fail;
    }
    return obj;

fail:
    ici_decref(s);
    return NULL;
}

// regexp

static ici_obj_t *
restore_regexp(ici_archive_t *ar)
{
    ici_obj_t *r;
    int options;
    ici_str_t *s;
    ici_obj_t *name;

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
        ici_decref(s);
        return NULL;
    }
    ici_decref(s);
    if (ici_archive_insert(ar, name, r))
    {
        ici_decref(r);
        return NULL;
    }
    return r;
}

// mem

static ici_obj_t *
restore_mem(ici_archive_t *ar)
{
    long len;
    int16_t accessz;
    long sz;
    void *p;
    ici_mem_t *m = 0;
    ici_obj_t *name;

    if (restore_object_name(ar, &name) || readl(ar, &len) || read16(ar, &accessz))
    {
        return NULL;
    }
    sz = len * accessz;
    if ((p = ici_alloc(sz)) != NULL)
    {
        if ((m = ici_mem_new(p, len, accessz, ici_free)) == NULL)
        {
            ici_free(p);
        }
        else if (readf(ar, p, sz) || ici_archive_insert(ar, name, m))
        {
            ici_decref(m);
            m = NULL;
        }
    }
    return m;
}

// array

static ici_obj_t *
restore_array(ici_archive_t *ar)
{
    long n;
    ici_array_t *a;
    ici_obj_t *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (readl(ar, &n))
    {
        return NULL;
    }
    if ((a = ici_array_new(n)) == NULL)
    {
        return NULL;
    }
    if (ici_archive_insert(ar, name, a))
    {
        goto fail;
    }
    for (; n > 0; --n)
    {
        ici_obj_t *o;

        if ((o = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if (ici_array_push(a, o))
        {
            ici_decref(o);
            goto fail1;
        }
	ici_decref(o);
    }
    return a;

fail1:
    ici_archive_uninsert(ar, name);

fail:
    ici_decref(a);
    return NULL;
}

// set

static ici_obj_t *
restore_set(ici_archive_t *ar)
{
    ici_set_t *s;
    long n;
    long i;
    ici_obj_t *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = ici_set_new()) == NULL)
    {
        return NULL;
    }
    if (ici_archive_insert(ar, name, s))
    {
        goto fail;
    }
    if (readl(ar, &n))
    {
        goto fail1;
    }
    for (i = 0; i < n; ++i)
    {
        ici_obj_t *o;

        if ((o = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if (ici_assign(s, o, ici_one))
        {
            ici_decref(o);
            goto fail1;
        }
        ici_decref(o);
    }
    return s;

fail1:
    ici_archive_uninsert(ar, name);

fail:
    ici_decref(s);
    return NULL;
}

// struct

static ici_obj_t *
restore_struct(ici_archive_t *ar)
{
    ici_struct_t *s;
    ici_obj_t *super;
    long n;
    long i;
    ici_obj_t *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = ici_struct_new()) == NULL)
    {
        return NULL;
    }
    if (ici_archive_insert(ar, name, s))
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
        ici_decref(super);
    }

    if (readl(ar, &n))
    {
        goto fail1;
    }
    for (i = 0; i < n; ++i)
    {
        ici_obj_t *key;
        ici_obj_t *value;
        int failed;

        if ((key = restore(ar)) == NULL)
        {
            goto fail1;
        }
        if ((value = restore(ar)) == NULL)
        {
            ici_decref(key);
            goto fail1;
        }
        failed = ici_assign(s, key, value);
        ici_decref(key);
        ici_decref(value);
        if (failed)
        {
            goto fail1;
        }
    }
    return s;

fail1:
    ici_archive_uninsert(ar, name);

fail:
    ici_decref(s);
    return NULL;
}

// ptr

static ici_obj_t *
restore_ptr(ici_archive_t *ar)
{
    ici_obj_t *aggr;
    ici_obj_t *key;
    ici_ptr_t *ptr;

    if ((aggr = restore(ar)) == NULL)
    {
        return NULL;
    }
    if ((key = restore(ar)) == NULL)
    {
        ici_decref(aggr);
        return NULL;
    }
    ptr = ici_ptr_new(aggr, key);
    ici_decref(aggr);
    ici_decref(key);
    return ptr ? ptr : NULL;
}

// func

static ici_obj_t *
restore_func(ici_archive_t *ar)
{
    ici_obj_t *code;
    ici_obj_t *args = NULL;
    ici_obj_t *autos = NULL;
    ici_obj_t *name = NULL;
    long nautos;
    ici_func_t *fn;
    ici_obj_t *oname;

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
    if (readl(ar, &nautos))
    {
        goto fail;
    }
    if ((fn = ici_new_func()) == NULL)
    {
        goto fail;
    }

    fn->f_code = ici_arrayof(code);
    fn->f_args = ici_arrayof(args);
    fn->f_autos = ici_structof(autos);
    fn->f_autos->o_super = ar->a_scope; /* structof(ici_vs.a_top[-1])->o_super; */
    fn->f_name = ici_stringof(name);
    fn->f_nautos = nautos;

    ici_decref(code);
    ici_decref(args);
    ici_decref(autos);
    ici_decref(name);

    if (!ici_archive_insert(ar, oname, fn))
    {
	return fn;
    }

    /*FALLTHROUGH*/

fail:
    ici_decref(code);
    if (args)
    {
        ici_decref(args);
    }
    if (autos)
    {
        ici_decref(autos);
    }
    if (name)
    {
        ici_decref(name);
    }
    return NULL;
}

static ici_obj_t *
restore_op(ici_archive_t *ar)
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

    return ici_new_op(ici_archive_op_func(op_func_code), op_ecode, op_code);
}

static ici_obj_t *
restore_src(ici_archive_t *ar)
{
    long line;
    ici_obj_t *result;
    ici_obj_t *filename;

    if (readl(ar, &line))
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
        ici_decref(filename);
        return NULL;
    }
    if ((result = ici_src_new(line, ici_stringof(filename))) == NULL)
    {
        ici_decref(filename);
        return NULL;
    }
    ici_decref(filename);
    return result;
}

// cfunc

static ici_obj_t *
restore_cfunc(ici_archive_t *ar)
{
    int16_t namelen;
    char space[32];
    char *buf;
    ici_str_t *func_name;
    ici_obj_t *fn;

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
    fn = ici_fetch(ar->a_scope, func_name);
    ici_decref(func_name);
    return fn;
}

static ici_obj_t *
restore_mark(ici_archive_t *ar)
{
    return &ici_o_mark;
}

// ref

static ici_obj_t *
restore_ref(ici_archive_t *ar)
{
    ici_obj_t *obj;
    ici_obj_t *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((obj = ici_archive_lookup(ar, name)) == ici_null)
    {
        obj = NULL;
    }
    return obj;
}

// restorer

struct restorer : object
{
    ici_obj_t *(*r_fn)(ici_archive_t *);
};

typedef struct restorer restorer_t;

unsigned long restorer_type::mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (restorer_t);
}

void restorer_type::free(ici_obj_t *o)
{
    ici_tfree(o, restorer_t);
}

static restorer_t *
restorer_new(ici_obj_t *(*fn)(ici_archive_t *))
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

static ici_struct_t *restorer_map = 0;

static int
add_restorer(int tcode, ici_obj_t *(*fn)(ici_archive_t *))
{
    restorer_t *r;
    ici_int_t *t = 0;

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
        ici_decref(t);
        goto fail;
    }
    ici_decref(t);
    ici_decref(r);
    return 0;

fail:
    ici_decref(r);
    return 1;
}

static restorer_t *
fetch_restorer(int key)
{
    ici_obj_t   *k;
    ici_obj_t   *v = NULL;

    if ((k = ici_int_new(key)) != NULL)
    {
        v = ici_fetch(restorer_map, k);
        ici_decref(k);
    }
    return (restorer_t *)v;
}

void
ici_uninit_restorer_map()
{
    ici_decref(restorer_map);
}

int
ici_init_restorer_map()
{
    size_t i;

    static struct
    {
        int tcode;
        ici_obj_t *(*fn)(ici_archive_t *);
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
            ici_decref(restorer_map);
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
    if (ici_isnull(r))
    {
        r = fetch_restorer(-1);
        if (ici_isnull(r))
        {
            ici_set_error("archive module internal error");
            r = NULL;
        }
    }
    return r;
}

static ici_obj_t *
restore(ici_archive_t *ar)
{
    ici_obj_t *obj = NULL;
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
ici_archive_f_restore(...)
{
    ici_file_t *file;
    ici_archive_t *ar;
    ici_objwsup_t *scp;
    ici_obj_t *obj = NULL;

    scp = ici_structof(ici_vs.a_top[-1])->o_super;
    switch (ICI_NARGS())
    {
    case 0:
        if ((file = ici_need_stdin()) == NULL)
        {
            return 1;
        }
	break;

    case 1:
        if (ici_typecheck("u", &file))
	{
            if (ici_typecheck("d", &scp))
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
	if (ici_typecheck("ud", &file, &scp))
        {
	    return 1;
        }
	break;
    }

    if ((ar = ici_archive_start(file, scp)) != NULL)
    {
        obj = restore(ar);
        ici_archive_stop(ar);
    }

    return obj == NULL ? 1 : ici_ret_with_decref(obj);
}

} // namespace ici

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
#include "archiver.h"
#include "int.h"
#include "float.h"
#include "array.h"
#include "map.h"
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

static object *restore(archiver *);

static int
restore_type(archiver *ar, char *flags)
{
    char tcode;

    if (ar->read(tcode))
    {
    	return -1;
    }
    *flags = tcode & O_ARCHIVE_ATOMIC ? object::O_ATOM : 0;
    tcode &= ~O_ARCHIVE_ATOMIC;
    return tcode;
}

inline int restore_object_name(archiver *ar, object **name)
{
    int64_t ref;
    if (ar->read(ref)) {
        return 1;
    }
    *name = (object *)ref;
    return 0;
}

static object *
restore_error(archiver *)
{
    set_error("unable to restore object");
    return NULL;
}

// null

static object *
restore_null(archiver *)
{
    return ici_null;
}

// int

static object *
restore_int(archiver *ar)
{
    int64_t value;

    if (ar->read(value))
    {
        return NULL;
    }
    return new_int(value);
}

// float

static object *
restore_float(archiver *ar)
{
    double val;
    if (ar->read(val)) {
        return NULL;
    }
    return new_float(val);
}

// string

static object *
restore_string(archiver *ar)
{
    str *s;
    int32_t len;
    object *name;
    object *obj;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (ar->read(len))
    {
        return NULL;
    }
    if ((s = str_alloc(len)) == NULL)
    {
        return NULL;
    }
    if (ar->read(s->s_chars, len))
    {
        goto fail;
    }
    hash_string(s);
    if ((obj = atom(s, 1)) == NULL)
    {
        goto fail;
    }
    if (ar->record(name, obj))
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
restore_regexp(archiver *ar)
{
    object *r;
    int32_t options;
    str *s;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (ar->read(options))
    {
        return NULL;
    }
    if ((s = stringof(restore_string(ar))) == NULL)
    {
        return NULL;
    }
    r = new_regexp(s, options);
    if (r == NULL)
    {
        s->decref();
        return NULL;
    }
    s->decref();
    if (ar->record(name, r))
    {
        r->decref();
        return NULL;
    }
    return r;
}

// mem

static object *
restore_mem(archiver *ar)
{
    int64_t len;
    int16_t accessz;
    size_t sz;
    void *p;
    mem *m = 0;
    object *name;

    if (restore_object_name(ar, &name) || ar->read(len) || ar->read(accessz))
    {
        return NULL;
    }
    sz = size_t(len) * size_t(accessz);
    if ((p = ici_alloc(sz)) != NULL)
    {
        if ((m = new_mem(p, len, accessz, ici_free)) == NULL)
        {
            ici_free(p);
        }
        else if (ar->read(p, sz) || ar->record(name, m))
        {
            m->decref();
            m = NULL;
        }
    }
    return m;
}

// array

static object *
restore_array(archiver *ar)
{
    int64_t n;
    array *a;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if (ar->read(n))
    {
        return NULL;
    }
    if ((a = new_array(n)) == NULL)
    {
        return NULL;
    }
    if (ar->record(name, a))
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
        if (a->push_back(o))
        {
            o->decref();
            goto fail1;
        }
	o->decref();
    }
    return a;

fail1:
    ar->remove(name);

fail:
    a->decref();
    return NULL;
}

// set

static object *
restore_set(archiver *ar)
{
    set *s;
    int64_t n;
    int64_t i;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = new_set()) == NULL)
    {
        return NULL;
    }
    if (ar->record(name, s))
    {
        goto fail;
    }
    if (ar->read(n))
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
        if (ici_assign(s, o, o_one))
        {
            o->decref();
            goto fail1;
        }
        o->decref();
    }
    return s;

fail1:
    ar->remove(name);

fail:
    s->decref();
    return NULL;
}

// struct

static object *restore_map(archiver *ar)
{
    map *s;
    object *super;
    int64_t n;
    int64_t i;
    object *name;

    if (restore_object_name(ar, &name))
    {
        return NULL;
    }
    if ((s = new_map()) == NULL)
    {
        return NULL;
    }
    if (ar->record(name, s))
    {
        goto fail;
    }
    if ((super = restore(ar)) == NULL)
    {
        goto fail1;
    }
    if (super != ici_null)
    {
        objwsupof(s)->o_super = objwsupof(super);
        super->decref();
    }

    if (ar->read(n))
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
    ar->remove(name);

fail:
    s->decref();
    return NULL;
}

// ptr

static object *
restore_ptr(archiver *ar)
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
    ptr = new_ptr(aggr, key);
    aggr->decref();
    key->decref();
    return ptr ? ptr : NULL;
}

// func

static object *
restore_func(archiver *ar)
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
    if (ar->read(nautos))
    {
        goto fail;
    }
    if ((fn = new_func()) == NULL)
    {
        goto fail;
    }

    fn->f_code = arrayof(code);
    fn->f_args = arrayof(args);
    fn->f_autos = mapof(autos);
    fn->f_autos->o_super = ar->scope(); /* mapof(vs.a_top[-1])->o_super; */
    fn->f_name = stringof(name);
    fn->f_nautos = nautos;

    code->decref();
    args->decref();
    autos->decref();
    name->decref();

    if (!ar->record(oname, fn))
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
restore_op(archiver *ar)
{
    int16_t op_func_code;
    int16_t op_ecode;
    int16_t op_code;

    if
    (
        ar->read(op_func_code)
        ||
        ar->read(op_ecode)
        ||
        ar->read(op_code)
    )
    {
        return nullptr;
    }

    return new_op(archiver::op_func(op_func_code), op_ecode, op_code);
}

static object *
restore_src(archiver *ar)
{
    int32_t line;
    object *result;
    object *filename;

    if (ar->read(line))
    {
        return NULL;
    }
    if ((filename = restore(ar)) == NULL)
    {
        return NULL;
    }
    if (!isstring(filename))
    {
        set_error("unexpected filename type (%s)", filename->type_name());
        filename->decref();
        return NULL;
    }
    if ((result = new_src(line, stringof(filename))) == NULL)
    {
        filename->decref();
        return NULL;
    }
    filename->decref();
    return result;
}

// cfunc

static object *
restore_cfunc(archiver *ar)
{
    int16_t namelen;
    char space[48];
    char *buf;
    str *func_name;
    object *fn;

    if (ar->read(namelen))
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
    if (ar->read(buf, namelen))
    {
        ici_free(buf);
        return 0;
    }
    if ((func_name = new_str(buf, namelen)) == NULL)
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
restore_mark(archiver *)
{
    return &o_mark;
}

// ref

static object *
restore_ref(archiver *ar)
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

static restorer *
restorer_new(object *(*fn)(archiver *))
{
    restorer *r;

    if ((r = ici_talloc(restorer)) != NULL)
    {
        set_tfnz(r, TC_RESTORER, 0, 1, sizeof (restorer));
        r->r_fn = fn;
        rego(r);
    }
    return r;
}

static map *restorer_map = 0;

static int
add_restorer(int tcode, object *(*fn)(archiver *))
{
    restorer *r;
    ici_int *t = 0;

    if ((r = restorer_new(fn)) == NULL)
    {
        return 1;
    }
    if ((t = new_int(tcode)) == NULL)
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

static restorer *
fetch_restorer(int key)
{
    object   *k;
    object   *v = NULL;

    if ((k = new_int(key)) != NULL)
    {
        v = ici_fetch(restorer_map, k);
        k->decref();
    }
    return (restorer *)v;
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
        object *(*fn)(archiver *);
    }
    fns[] =
    {
        {-1,        restore_error},
        {TC_NULL,   restore_null},
        {TC_INT,    restore_int},
        {TC_FLOAT,  restore_float},
        {TC_STRING, restore_string},
        {TC_REGEXP, restore_regexp},
        {TC_MEM,    restore_mem},
        {TC_ARRAY,  restore_array},
        {TC_SET,    restore_set},
        {TC_MAP,    restore_map},
        {TC_PTR,    restore_ptr},
        {TC_FUNC,   restore_func},
        {TC_OP,     restore_op},
        {TC_SRC,    restore_src},
        {TC_CFUNC,  restore_cfunc},
        {TC_MARK,   restore_mark},
        {TC_REF,    restore_ref}
    };

    if ((restorer_map = new_map()) == NULL)
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

static restorer *
get_restorer(int tcode)
{
    restorer  *r = fetch_restorer(tcode);
    if (isnull(r))
    {
        r = fetch_restorer(-1);
        if (isnull(r))
        {
            set_error("archive module internal error");
            r = NULL;
        }
    }
    return r;
}

static object *
restore(archiver *ar)
{
    object *obj = nullptr;
    char flags;
    int tcode;

    if ((tcode = restore_type(ar, &flags)) != -1)
    {
        if (auto r = get_restorer(tcode)) {
            if ((obj = (*r->r_fn)(ar)) != nullptr) {
                if (flags & object::O_ATOM) {
                    obj = atom(obj, 1);
                }
            }
        }
    }
    return obj;
}

int
f_archive_restore(...)
{
    file *file;
    objwsup *scp;
    object *obj = NULL;

    scp = mapof(vs.a_top[-1])->o_super;
    switch (NARGS())
    {
    case 0:
        if ((file = need_stdin()) == NULL)
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
	    if ((file = need_stdin()) == NULL)
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

    {
        archiver ar(file, scp);
        if (ar) {
            obj = restore(&ar);
        }
    }

    return obj == NULL ? 1 : ret_with_decref(obj);
}

} // namespace ici

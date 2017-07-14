#define ICI_CORE

#include "fwd.h"
#include "saver.h"

#include "archive.h"
#include "array.h"
#include "cfunc.h"
#include "file.h"
#include "int.h"
#include "null.h"
#include "float.h"
#include "func.h"
#include "mark.h"
#include "mem.h"
#include "op.h"
#include "ptr.h"
#include "src.h"
#include "str.h"
#include "map.h"
#include "set.h"
#include "re.h"
#include "pcre.h"

#include <netinet/in.h>

namespace ici
{

static int archive_save(archiver *ar, object *obj);

/*
 * Strings are written as a long count of the number of bytes followed
 * by that many bytes representing the character data.  Since ICI uses
 * 8-bit coding this count is also the number of characters.
 */
inline int write(archiver *ar, str *s)
{
    return ar->write(int32_t(s->s_nchars)) || ar->write(s->s_chars, s->s_nchars);
}

/*
 * Reference to a previously saved object
 */
static int save_object_ref(archiver *ar, object *o)
{
    return ar->write(TC_REF) || ar->write(&o, sizeof o);
}

static int
save_object_name(archiver *ar, object *obj)
{
    return ar->record(obj, obj) || ar->write(&obj, sizeof obj);
}

/*
 * ici object header
 */

static int
save_obj(archiver *ar, object *obj)
{
    uint8_t code = obj->o_tcode & 0x1F;
    if (obj->isatom())
        code |= O_ARCHIVE_ATOMIC;
    return ar->write(code);
}

// NULL

static int
save_null(archiver *, object *)
{
    return 0;
}

// int

static int
save_int(archiver *ar, object *obj)
{
    return ar->write(intof(obj)->i_value);
}

// float

static int
save_float(archiver *ar, object *obj)
{
    return ar->write(floatof(obj)->f_value);
}

// string

static int
save_string(archiver *ar, object *obj)
{
    return save_object_name(ar, obj) || write(ar, stringof(obj));
}

// regexp

static int
save_regexp(archiver *ar, object *obj)
{
    auto re = regexpof(obj);
    int options;
    ici_pcre_info(re->r_re, &options, NULL);
    return save_object_name(ar, obj) || ar->write(options) || save_string(ar, re->r_pat);
}

// mem

static int
save_mem(archiver *ar, object *obj)
{
    auto m = memof(obj);
    return save_object_name(ar, obj)
        || ar->write(int64_t(m->m_length))
        || ar->write(int16_t(m->m_accessz))
        || ar->write(m->m_base, m->m_length * m->m_accessz);
}

// array

static int
save_array(archiver *ar, object *obj)
{
    array *a = arrayof(obj);
    object **e;

    if (save_object_name(ar, obj) || ar->write(int64_t(a->len())))
        return 1;
    for (e = a->astart(); e != a->alimit(); e = a->anext(e))
    {
        if (archive_save(ar, *e))
            return 1;
    }
    return 0;
}

// set

static int
save_set(archiver *ar, object *obj)
{
    auto s = setof(obj);
    object **e = s->s_slots;

    if (save_object_name(ar, obj) || ar->write(int64_t(s->s_nels)))
        return 1;
    for (; size_t(e - s->s_slots) < s->s_nslots; ++e)
    {
        if (*e && archive_save(ar, *e))
            return 1;
    }
    return 0;
}

// struct

static int
save_map(archiver *ar, object *obj)
{
    map *s = mapof(obj);
    object *super = objwsupof(s)->o_super;
    slot *sl;
    if (super == nullptr) {
        super = ici_null;
    }
    if (save_object_name(ar, obj) || archive_save(ar, super) || ar->write(int64_t(s->s_nels)))
        return 1;
    for (sl = s->s_slots; size_t(sl - s->s_slots) < s->s_nslots; ++sl)
    {
        if (sl->sl_key && sl->sl_value)
        {
            if (archive_save(ar, sl->sl_key) || archive_save(ar, sl->sl_value))
                return 1;
        }
    }
    return 0;
}

// ptr

static int
save_ptr(archiver *ar, object *obj)
{
    return archive_save(ar, ptrof(obj)->p_aggr) || archive_save(ar, ptrof(obj)->p_key);
}

// func

static int
save_func(archiver *ar, object *obj)
{
    auto f = funcof(obj);
    map *autos;

    if (iscfunc(obj))
    {
        auto cf = cfuncof(obj);
        return ar->write(int16_t(cf->cf_name->s_nchars)) || ar->write(cf->cf_name->s_chars, cf->cf_name->s_nchars);
    }

    if (save_object_name(ar, obj) || archive_save(ar, f->f_code) || archive_save(ar, f->f_args))
        return 1;
    if ((autos = mapof(f->f_autos->copy())) == NULL)
        return 1;
    autos->o_super = NULL;
    unassign(autos, SS(_func_));
    if (archive_save(ar, autos))
    {
        autos->decref();
        return 1;
    }
    autos->decref();

    return archive_save(ar, f->f_name) || ar->write(int32_t(f->f_nautos));
}

// src

static int
save_src(archiver *ar, object *obj)
{
    return ar->write(int32_t(srcof(obj)->s_lineno)) || archive_save(ar, srcof(obj)->s_filename);
}

// op

static int
save_op(archiver *ar, object *obj)
{
    return
        ar->write(int16_t(archive_op_func_code(opof(obj)->op_func)))
        ||
        ar->write(int16_t(opof(obj)->op_ecode))
        ||
        ar->write(int16_t(opof(obj)->op_code));
}

//
// saver
//

static object *
new_saver(int (*fn)(archiver *, object *))
{
    saver *s;
    if ((s = ici_talloc(saver)) != NULL)
    {
        set_tfnz(s, TC_SAVER, 0, 1, sizeof (saver));
        s->s_fn = fn;
        rego(s);
    }
    return s;
}

static map *saver_map = NULL;

void
uninit_saver_map()
{
    saver_map->decref();
}

int
init_saver_map()
{
    size_t i;
    struct
    {
        object *name;
        int (*fn)(archiver *, object *);
    }
    fns[] =
    {
        {SS(_NULL_), save_null},
        {SS(mark), save_null},
        {SS(int), save_int},
        {SS(float), save_float},
        {SS(string), save_string},
        {SS(regexp), save_regexp},
        {SS(array), save_array},
        {SS(set), save_set},
        {SS(map), save_map},
        {SS(mem), save_mem},
        {SS(ptr), save_ptr},
        {SS(func), save_func},
        {SS(src), save_src},
        {SS(op), save_op},
    };

    if ((saver_map = new_map()) == NULL)
    {
        return 1;
    }
    for (i = 0; i < nels(fns); ++i)
    {
        object *saver;

        if ((saver = new_saver(fns[i].fn)) == NULL)
            goto fail;
        if (ici_assign(saver_map, fns[i].name, saver))
        {
            saver->decref();
            goto fail;
        }
        saver->decref();
    }
    return 0;

fail:
    saver_map->decref();
    saver_map = NULL;
    return 1;
}

static str *
tname(object *o)
{
    return o->otype()->ici_name();
}

static int
save_error(archiver *, object *obj)
{
    return set_error("%s: unable to save type", obj->type_name());
}


static int
archive_save(archiver *ar, object *obj)
{
    object *saver;
    int (*fn)(archiver *, object *);

    if (ar->lookup(obj) != NULL)
    {
        return save_object_ref(ar, obj);
    }
    saver = ici_fetch(saver_map, tname(obj));
    fn = saver == ici_null ? save_error : saverof(saver)->s_fn;
    return save_obj(ar, obj) || (*fn)(ar, obj);
}

/*
 * save([file, ] any)
 *
 * Save an object to a file by writing a serialized object graph
 * using the given object as the root of the graph.
 *
 * If file is not given the object is written to the standard output.
 *
 * This --topic-- forms part of the --ici-serialisation-- documentation.
 */
int f_archive_save(...)
{
    objwsup *scp = mapof(vs.a_top[-1])->o_super;
    file *file;
    object *obj;
    int failed = 1;

    switch (NARGS())
    {
    case 3:
        if (typecheck("uod", &file, &obj, &scp))
            return 1;
        break;

    case 2:
        if (typecheck("uo", &file, &obj))
            return 1;
        break;

    case 1:
        if (typecheck("o", &obj))
            return 1;
        if ((file = need_stdout()) == NULL)
            return 1;
        break;

    default:
        return argerror(2);
    }

    archiver ar(file, scp);
    if (ar) {
        failed = archive_save(&ar, obj);
    }
    return failed ? failed : null_ret();
}

} // namespace ici

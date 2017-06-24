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
#include "struct.h"
#include "set.h"
#include "re.h"
#include "pcre.h"

#include <netinet/in.h>

namespace ici
{

static int archive_save(archive *ar, object *obj);

/*
 * Functions to write different size datums to the archive stream.
 */

inline int writef(archive *ar, const void *data, int len)
{
    return ar->write(data, len) != len;
}

inline int writeb(archive *ar, unsigned char abyte)
{
    return writef(ar, &abyte, 1);
}

inline int write16(archive *ar, int16_t hword)
{
    int16_t swapped = htons(hword);
    return writef(ar, &swapped, sizeof swapped);
}

inline int write32(archive *ar, int32_t aword)
{
    int32_t swapped = htonl(aword);
    return writef(ar, &swapped, sizeof swapped);
}

inline int write64(archive *ar, int64_t dword)
{
    auto swapped = ici_htonll(dword);
    return writef(ar, &swapped, sizeof swapped);
}

inline int writedbl(archive *ar, double adbl)
{
    return writef(ar, &adbl, sizeof adbl);
}

/*
 * Strings are written as a long count of the number of bytes followed
 * by that many bytes representing the character data.  Since ICI uses
 * 8-bit coding this count is also the number of characters.
 */
inline int writestr(archive *ar, str *s)
{
    return write32(ar, s->s_nchars) || writef(ar, s->s_chars, s->s_nchars);
}

/*
 * Reference to a previously saved object
 */
static int save_object_ref(archive *ar, object *o)
{
    return writeb(ar, TC_REF) || writef(ar, &o, sizeof o);
}

static int
save_object_name(archive *ar, object *obj)
{
    return ar->insert(obj, obj) || writef(ar, &obj, sizeof obj);
}

/*
 * ici object header
 */

static int
save_obj(archive *ar, object *obj)
{
    uint8_t code = obj->o_tcode & 0x1F;
    if (obj->isatom())
        code |= O_ARCHIVE_ATOMIC;
    return writeb(ar, code);
}

// NULL

static int
save_null(archive *, object *)
{
    return 0;
}

// int

static int
save_int(archive *ar, object *obj)
{
    return write64(ar, intof(obj)->i_value);
}

// float

static int
save_float(archive *ar, object *obj)
{
    double v = floatof(obj)->f_value;
#if ICI_ARCHIVE_LITTLE_ENDIAN_HOST
    archive_byteswap(&v, sizeof v);
#endif
    return writedbl(ar, v);
}

// string

static int
save_string(archive *ar, object *obj)
{
    return save_object_name(ar, obj) || writestr(ar, stringof(obj));
}

// regexp

static int
save_regexp(archive *ar, object *obj)
{
    auto re = regexpof(obj);
    int options;
    ici_pcre_info(re->r_re, &options, NULL);
    return save_object_name(ar, obj) || write32(ar, options) || save_string(ar, re->r_pat);
}

// mem

static int
save_mem(archive *ar, object *obj)
{
    auto m = memof(obj);
    return save_object_name(ar, obj)
           || write64(ar, m->m_length)
           || write16(ar, m->m_accessz)
           || writef(ar, m->m_base, m->m_length * m->m_accessz);
}

// array

static int
save_array(archive *ar, object *obj)
{
    array *a = arrayof(obj);
    object **e;

    if (save_object_name(ar, obj) || write64(ar, a->len()))
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
save_set(archive *ar, object *obj)
{
    auto s = setof(obj);
    object **e = s->s_slots;

    if (save_object_name(ar, obj) || write64(ar, s->s_nels))
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
save_struct(archive *ar, object *obj)
{
    ici_struct *s = structof(obj);
    object *super = objwsupof(s)->o_super;
    struct sslot *sl;
    if (super == nullptr) {
        super = ici_null;
    }
    if (save_object_name(ar, obj) || archive_save(ar, super) || write64(ar, s->s_nels))
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
save_ptr(archive *ar, object *obj)
{
    return archive_save(ar, ptrof(obj)->p_aggr) || archive_save(ar, ptrof(obj)->p_key);
}

// func

static int
save_func(archive *ar, object *obj)
{
    auto f = funcof(obj);
    ici_struct *autos;

    if (iscfunc(obj))
    {
        auto cf = cfuncof(obj);
        return write16(ar, cf->cf_name->s_nchars) || writef(ar, cf->cf_name->s_chars, cf->cf_name->s_nchars);
    }

    if (save_object_name(ar, obj) || archive_save(ar, f->f_code) || archive_save(ar, f->f_args))
        return 1;
    if ((autos = structof(f->f_autos->copy())) == NULL)
        return 1;
    autos->o_super = NULL;
    unassign(autos, SS(_func_));
    if (archive_save(ar, autos))
    {
        autos->decref();
        return 1;
    }
    autos->decref();

    return archive_save(ar, f->f_name) || write32(ar, f->f_nautos);
}

// src

static int
save_src(archive *ar, object *obj)
{
    return write32(ar, srcof(obj)->s_lineno) || archive_save(ar, srcof(obj)->s_filename);
}

// op

static int
save_op(archive *ar, object *obj)
{
    return
        write16(ar, archive_op_func_code(opof(obj)->op_func))
        ||
        write16(ar, opof(obj)->op_ecode)
        ||
        write16(ar, opof(obj)->op_code);
}

//
// saver
//

static object *
new_saver(int (*fn)(archive *, object *))
{
    saver_t *saver;

    if ((saver = ici_talloc(saver_t)) != NULL)
    {
        set_tfnz(saver, TC_SAVER, 0, 1, sizeof (saver_t));
        saver->s_fn = fn;
        rego(saver);
    }

    return saver;
}

static ici_struct *saver_map = NULL;

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
        int (*fn)(archive *, object *);
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
        {SS(struct), save_struct},
        {SS(mem), save_mem},
        {SS(ptr), save_ptr},
        {SS(func), save_func},
        {SS(src), save_src},
        {SS(op), save_op},
    };

    if ((saver_map = new_struct()) == NULL)
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
save_error(archive *, object *obj)
{
    return set_error("%s: unable to save type", obj->type_name());
}


static int
archive_save(archive *ar, object *obj)
{
    object *saver;
    int (*fn)(archive *, object *);

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
    objwsup *scp = structof(vs.a_top[-1])->o_super;
    file *file;
    object *obj;
    archive *ar;
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

    if ((ar = archive::start(file, scp)) != NULL)
    {
        failed = archive_save(ar, obj);
        ar->stop();
    }

    return failed ? failed : null_ret();
}

} // namespace ici

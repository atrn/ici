#define ICI_CORE

#include "fwd.h"
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
#include "pcre/pcre.h"

#include <netinet/in.h>

namespace ici
{

static int ici_archive_save(ici_archive_t *ar, ici_obj_t *obj);

/*
 * Functions to write different size datums to the archive stream.
 */

inline
static int
writef(ici_archive_t *ar, const void *data, int len)
{
    return (*ar->a_file->f_type->ft_write)(data, len, ar->a_file->f_file) != len;
}

inline
static int
writeb(ici_archive_t *ar, unsigned char abyte)
{
    return writef(ar, &abyte, 1);
}

inline
static int
write32(ici_archive_t *ar, int aword)
{
    int swapped = htonl(aword);
    return writef(ar, &swapped, sizeof swapped);
}

inline
static int
writedbl(ici_archive_t *ar, double adbl)
{
    return writef(ar, &adbl, sizeof adbl);
}

/*
 * Integers are written as "long" in network byte order.
 */
inline
static int
writel(ici_archive_t *ar, long along)
{
    long swapped = htonl(along);
    return writef(ar, &swapped, sizeof swapped);
}

/*
 * Strings are written as a long count of the number of bytes followed
 * by that many bytes representing the character data.  Since ICI uses
 * 8-bit coding this count is also the number of characters.
 */
inline
static int
writestr(ici_archive_t *ar, ici_str_t *s)
{
    return writel(ar, s->s_nchars) || writef(ar, s->s_chars, s->s_nchars);
}

/*
 * Reference to a previously saved object
 */

static int
save_object_ref(ici_archive_t *ar, ici_obj_t *o)
{
    return writeb(ar, ICI_TC_REF) || writef(ar, &o, sizeof o);
}

static int
save_object_name(ici_archive_t *ar, ici_obj_t *obj)
{
    return ici_archive_insert(ar, obj, obj) || writef(ar, &obj, sizeof obj);
}

/*
 * ici object header
 */

static int
save_obj(ici_archive_t *ar, ici_obj_t *obj)
{
    unsigned char code = obj->o_tcode & 0x1F;
    if (obj->o_flags & ICI_O_ATOM)
        code |= ICI_ARCHIVE_ATOMIC;
    return writeb(ar, code);
}

// NULL

static int
save_null(ici_archive_t *ar, ici_obj_t *obj)
{
    (void)ar;
    (void)obj;
    return 0;
}

// int

static int
save_int(ici_archive_t *ar, ici_obj_t *obj)
{
    return writel(ar, ici_intof(obj)->i_value);
}

// float

static int
save_float(ici_archive_t *ar, ici_obj_t *obj)
{
    double v = ici_floatof(obj)->f_value;
#if ICI_ARCHIVE_LITTLE_ENDIAN_HOST
    ici_archive_byteswap(&v, sizeof v);
#endif
    return writedbl(ar, v);
}

// string

static int
save_string(ici_archive_t *ar, ici_obj_t *obj)
{
    return save_object_name(ar, obj) || writestr(ar, ici_stringof(obj));
}

// regexp

static int
save_regexp(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_regexp_t *re = ici_regexpof(obj);
    int options;

    ici_pcre_info(re->r_re, &options, NULL);
    return save_object_name(ar, obj) || write32(ar, options) || save_string(ar, ici_objof(re->r_pat));
}

// mem

static int
save_mem(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_mem_t *m = ici_memof(obj);
    return save_object_name(ar, obj)
           || write32(ar, m->m_length)
           || write32(ar, m->m_accessz)
           || writef(ar, m->m_base, m->m_length * m->m_accessz);
}

// array

static int
save_array(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_array_t *a = ici_arrayof(obj);
    ici_obj_t **e;

    if (save_object_name(ar, obj) || writel(ar, ici_array_nels(a)))
        return 1;
    for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
    {
        if (ici_archive_save(ar, *e))
            return 1;
    }
    return 0;
}

// set

static int
save_set(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_set_t *s = ici_setof(obj);
    ici_obj_t **e = s->s_slots;

    if (save_object_name(ar, obj) || writel(ar, s->s_nels))
        return 1;
    for (; e - s->s_slots < s->s_nslots; ++e)
    {
        if (*e && ici_archive_save(ar, *e))
            return 1;
    }
    return 0;
}

// struct

static int
save_struct(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_struct_t *s = ici_structof(obj);
    ici_objwsup_t *super = ici_objwsupof(s)->o_super;
    struct ici_sslot *sl;

    if (save_object_name(ar, obj) || ici_archive_save(ar, super ? ici_objof(super) : ici_null) || writel(ar, s->s_nels))
        return 1;
    for (sl = s->s_slots; sl - s->s_slots < s->s_nslots; ++sl)
    {
        if (sl->sl_key && sl->sl_value)
        {
            if (ici_archive_save(ar, sl->sl_key) || ici_archive_save(ar, sl->sl_value))
                return 1;
        }
    }
    return 0;
}

// ptr

static int
save_ptr(ici_archive_t *ar, ici_obj_t *obj)
{
    return ici_archive_save(ar, ici_ptrof(obj)->p_aggr) || ici_archive_save(ar, ici_ptrof(obj)->p_key);
}

// func

static int
save_func(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_func_t *f = ici_funcof(obj);
    ici_struct_t *autos;

    if (obj->o_tcode == ICI_TC_CFUNC)
    {
        ici_cfunc_t *cf = ici_cfuncof(obj);
        int nchars = strlen(cf->cf_name);
        return write32(ar, nchars) || writef(ar, cf->cf_name, nchars);
    }

    if (save_object_name(ar, obj) || ici_archive_save(ar, ici_objof(f->f_code)) || ici_archive_save(ar, ici_objof(f->f_args)))
        return 1;
    if ((autos = ici_structof(ici_typeof(f->f_autos)->t_copy(ici_objof(f->f_autos)))) == NULL)
        return 1;
    autos->o_super = NULL;
    ici_struct_unassign(autos, SSO(_func_));
    if (ici_archive_save(ar, ici_objof(autos)))
    {
        ici_decref(autos);
        return 1;
    }
    ici_decref(autos);

    return ici_archive_save(ar, ici_objof(f->f_name)) || writel(ar, f->f_nautos);
}

// src

static int
save_src(ici_archive_t *ar, ici_obj_t *obj)
{
    return writel(ar, ici_srcof(obj)->s_lineno) || ici_archive_save(ar, ici_objof(ici_srcof(obj)->s_filename));
}

// op

static int
save_op(ici_archive_t *ar, ici_obj_t *obj)
{
    return
        writel(ar, ici_archive_op_func_code(ici_opof(obj)->op_func))
        ||
        writel(ar, ici_opof(obj)->op_ecode)
        ||
        writel(ar, ici_opof(obj)->op_code);
}

//
// saver
//

struct saver : ici_obj
{
    int (*s_fn)(ici_archive_t *, ici_obj_t *);
};

typedef struct saver saver_t;

static saver_t *
saverof(ici_obj_t *obj)
{
    return (saver_t *)obj;
}

static unsigned long
mark_saver(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (saver_t);
}

static void
free_saver(ici_obj_t *o)
{
    ici_tfree(o, saver_t);
}

ici_type_t
ici_saver_type =
{
    mark_saver,
    free_saver,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "saver"
};

static ici_obj_t *
new_saver(int (*fn)(ici_archive_t *, ici_obj_t *))
{
    saver_t *saver;

    if ((saver = ici_talloc(saver_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(saver, ICI_TC_SAVER, 0, 1, sizeof (saver_t));
        saver->s_fn = fn;
        ici_rego(saver);
    }

    return ici_objof(saver);
}

static ici_struct_t *saver_map = NULL;

int
ici_init_saver_map(void)
{
    size_t i;
    struct
    {
        ici_obj_t *name;
        int (*fn)(ici_archive_t *, ici_obj_t *);
    }
    fns[] =
    {
        {SSO(_NULL_),   save_null},
        {SSO(mark),     save_null},
        {SSO(int),      save_int},
        {SSO(float),    save_float},
        {SSO(string), save_string},
        {SSO(regexp), save_regexp},
        {SSO(array), save_array},
        {SSO(set), save_set},
        {SSO(struct), save_struct},
        {SSO(mem), save_mem},
        {SSO(ptr), save_ptr},
        {SSO(func), save_func},
        {SSO(src), save_src},
        {SSO(op), save_op},
    };

    if ((saver_map = ici_struct_new()) == NULL)
    {
        return 1;
    }
    for (i = 0; i < nels(fns); ++i)
    {
        ici_obj_t *saver;

        if ((saver = new_saver(fns[i].fn)) == NULL)
            goto fail;
        if (ici_assign(saver_map, fns[i].name, saver))
        {
            ici_decref(saver);
            goto fail;
        }
        ici_decref(saver);
    }
    return 0;

fail:
    ici_decref(saver_map);
    saver_map = NULL;
    return 1;
}

static ici_str_t *
tname(ici_obj_t *o)
{
    if (ici_typeof(o)->t_ici_name == NULL)
    {
        ici_typeof(o)->t_ici_name = ici_str_new_nul_term(ici_typeof(o)->t_name);
    }
    return ici_typeof(o)->t_ici_name;
}

static int
save_error(ici_archive_t *ar, ici_obj_t *obj)
{
    (void)ar;
    return ici_set_error("%s: unable to save type", ici_typeof(obj)->t_name);
}


static int
ici_archive_save(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_obj_t *saver;
    int (*fn)(ici_archive_t *, ici_obj_t *);

    if (ici_archive_lookup(ar, obj) != NULL)
    {
        return save_object_ref(ar, obj);
    }
    saver = ici_fetch(ici_objof(saver_map), ici_objof(tname(obj)));
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
int
ici_archive_f_save(...)
{
    ici_objwsup_t *scp = ici_structof(ici_vs.a_top[-1])->o_super;
    ici_file_t *file;
    ici_obj_t *obj;
    ici_archive_t *ar;
    int failed = 1;

    switch (ICI_NARGS())
    {
    case 3:
        if (ici_typecheck("uod", &file, &obj, &scp))
            return 1;
        break;

    case 2:
        if (ici_typecheck("uo", &file, &obj))
            return 1;
        break;

    case 1:
        if (ici_typecheck("o", &obj))
            return 1;
        if ((file = ici_need_stdout()) == NULL)
            return 1;
        break;

    default:
        return ici_argerror(2);
    }

    if ((ar = ici_archive_start(file, scp)) != NULL)
    {
        failed = ici_archive_save(ar, obj);
        ici_archive_stop(ar);
    }

    return failed ? failed : ici_null_ret();
}

} // namespace ici

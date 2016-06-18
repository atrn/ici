#define ICI_CORE

/*
 * Object Serialization
 *
 * The ICI 'archive' module seriialzes ICI object graphs, reading and
 * writing an encoded object representation - the ICI serialization
 * protocol.
 *
 * The two functions save and restore write and read objects. Both
 * functions may be passed an ICI file object used as the source or
 * destination of object data. The functions default to using the
 * standard input and output files as per other ICI I/O functions.
 *
 * All builtin object types apart from files may be serialized and
 * later restored, including functions. Functions are serialized as
 * their ICI VM byte code.  Native code functions, i.e. functions that
 * are implemented in C, aka "cfuncs", are saved by name.  During a
 * restore the function with that name is looked up in current scope.
 *
 * Object Serialization Protocol
 *
 * The ICI object serialization protocol is a tagged binary protocol.
 * The protocol is platform independent in that it defines the size
 * and format of all exchanged data.
 *
 * E.g. all integer types are represented in network byte order,
 * floating pointed is defined to be IEEE-754 doubles, sizes are sent
 * as 32-bit unsigned values, etc...  The exact protocol follows.
 *
 * tcode        byte, top-bit set if object is an ICI atom
 *
 * object ::- tcode object-specific-data ;
 *
 * null ::- tcode ;
 *
 * int ::- tcode 64-bit integer
 *
 * ... fixme ...
 *
 *
 * This --intro-- and --synopsis-- are part of --ici-serialisation-- documentation.
 */

#include "fwd.h"
#include "archive.h"
#include "cfunc.h"
#include "op.h"

typedef int int_func();
static int_func *op_funcs[7];

#define num_op_funcs ((int)(sizeof op_funcs / sizeof op_funcs[0]))

// static int ici_archive_tcode = ICI_TC_ARCHIVE;

inline static ici_archive_t *
archive_of(ici_obj_t *o)
{
    return (ici_archive_t *)(o);
}

static unsigned long
mark_archive(ici_obj_t *o)
{
    ici_archive_t *s = archive_of(o);
    o->o_flags |= ICI_O_MARK;
    return sizeof *s
        + ici_mark(s->a_file)
        + ici_mark(s->a_sent)
        + ici_mark(s->a_scope);
}

static void
free_archive(ici_obj_t *o)
{
    ici_tfree(o, ici_archive_t);
}

ici_type_t ici_archive_type =
{
    mark_archive,
    free_archive,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "archive"
};

int ici_archive_init(void)
{
    op_funcs[0] = NULL;
    op_funcs[1] = ici_o_mklvalue.op_func;
    op_funcs[2] = ici_o_onerror.op_func;
    op_funcs[3] = ici_o_return.op_func;
    op_funcs[4] = ici_o_mkptr.op_func;
    op_funcs[5] = ici_o_openptr.op_func;
    op_funcs[6] = ici_o_fetch.op_func;
    if (ici_init_saver_map())
    {
        return 1;
    }
    return ici_init_restorer_map();
}

static ici_archive_t *
new_archive(ici_file_t *file, ici_objwsup_t *scope)
{
    ici_archive_t *ar = ici_talloc(ici_archive_t);
    if (ar != NULL)
    {
        ICI_OBJ_SET_TFNZ(ar, ICI_TC_ARCHIVE, 0, 1, 0);
        if ((ar->a_sent = ici_struct_new()) == NULL)
        {
            ici_tfree(ar, ici_archive_t);
            return NULL;
        }
	ici_decref(ar->a_sent);
        ar->a_file = file;
	ar->a_scope = scope;
        ici_rego(ar);
    }
    return ar;
}

ici_archive_t *
ici_archive_start(ici_file_t *file, ici_objwsup_t *scope)
{
    ici_archive_t *ar = new_archive(file, scope);
    return ar;
}

inline static ici_obj_t *
make_key(ici_obj_t *obj)
{
    return ici_objof(ici_int_new((long)obj));
}

int
ici_archive_insert(ici_archive_t *ar, ici_obj_t *key, ici_obj_t *val)
{
    ici_obj_t *k;
    int failed = 1;

    if ((k = make_key(key)) != NULL)
    {
        failed = ici_assign(ar->a_sent, k, val);
        ici_decref(k);
    }
    return failed;
}

void
ici_archive_uninsert(ici_archive_t *ar, ici_obj_t *key)
{
    ici_obj_t *k;

    if ((k = make_key(key)) != NULL)
    {
        ici_struct_unassign(ar->a_sent, k);
        ici_decref(k);
    }
}

ici_obj_t *
ici_archive_lookup(ici_archive_t *ar, ici_obj_t *obj)
{
    ici_obj_t *v = ici_null;
    ici_obj_t *k;

    if ((k = make_key(obj)) != NULL)
    {
        v = ici_fetch(ar->a_sent, k);
        ici_decref(k);
    }
    return v == ici_null ? NULL : v;
}

void
ici_archive_stop(ici_archive_t *ar)
{
    ici_decref(ar);
}

int ici_archive_op_func_code(int_func *fn)
{
    int i;

    for (i = 0; i < num_op_funcs; ++i)
    {
        if (fn == op_funcs[i])
            return i;
    }
    return -1;
}

int_func *ici_archive_op_func(int code)
{
    if (code < 0 || code >= num_op_funcs)
    {
        return NULL;
    }
    return op_funcs[code];
}

void ici_archive_byteswap(void *ptr, int sz)
{
    if (sz == sizeof (long long))
    {
        long long *ll = ptr;
        *ll = htonll(*ll);
        return;
    }

    char *s = ptr;
    char *e = ptr + sz - 1;
    int i;

    for (i = 0; i < sz / 2 ; ++i)
    {
        char t = s[i];
        s[i] = e[-i];
        e[-i] = t;
    }
}

ici_cfunc_t ici_save_restore_cfuncs[] =
{
    {ICI_CF_OBJ, "save",        ici_archive_f_save,     0, 0},
    {ICI_CF_OBJ, "restore",     ici_archive_f_restore,  0, 0},
    {ICI_CF_OBJ}
};

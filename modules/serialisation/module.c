/*
 * Object Serialization
 *
 * The ICI 'archive' module performs serialization of ICI object graphs.
 * The module provides two functions, save and restore, to write and read
 * objects to and from files.  All builtin object types that do not rely
 * upon external state, everything but the "file" type used with OS files,
 * may be serialized to a file and later restored.  Files may be long
 * lived disk files, network connections or whatever file-like object.
 *
 * Special note needs to be made that function objects may be serialized.
 * ICI functions are written out as the byte code for the ICI VM (an array
 * of code objects and the like).  Native code functions, i.e. functions
 * that are implemented as native code and known as "cfunc" objects are
 * saved by simply saving their name.  When restored the current scope
 * is looked up for a similarly named cfunc object and that is assumed
 * to tbe write one.
 *
 * Object Serialization Protocol
 *
 * The ICI object serialization protocol is a simple, tagged binary
 * protocol for transferring ICI data types between writers and readers.
 * The protocol is platform independent in that it defines the size and
 * format of all types exchanged. E.g. all integer types are represented
 * in network byte order, floating pointed is defined to be IEEE-754
 * doubles, sizes are sent as 32-bit unsigned values, etc...  The exact
 * protocol follows.
 *
 * tcode        byte, top-bit set if object is atomic
 *
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

#include "archive.h"
#include "icistr.h"
#include <icistr-setup.h>

static int ici_archive_tcode = 0;

static ici_archive_t *
archive_of(ici_obj_t *o)
{
    return (ici_archive_t *)(o);
}

static unsigned long
mark_archive(ici_obj_t *o)
{
    ici_archive_t *s = archive_of(o);
    o->o_flags |= ICI_O_MARK;
    return sizeof *s + ici_mark(s->a_file) + ici_mark(s->a_sent) + (s->a_scope ? ici_mark(s->a_scope) : 0);
}

static void
free_archive(ici_obj_t *o)
{
    ici_tfree(o, ici_archive_t);
}

static ici_type_t ici_archive_type =
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

static ici_archive_t *
new_archive(ici_file_t *file, ici_objwsup_t *scope)
{
    ici_archive_t *ar = ici_talloc(ici_archive_t);
    if (ar != NULL)
    {
        ICI_OBJ_SET_TFNZ(ar, ici_archive_tcode, 0, 1, 0);
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

static ici_obj_t *
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

typedef int int_func();
static int_func *op_funcs[7];
static int init_op_funcs_table = 1;

#define NOPS ((int)(sizeof op_funcs / sizeof op_funcs[0]))

static void init_op_funcs(void)
{
    op_funcs[0] = NULL;
    op_funcs[1] = ici_o_mklvalue.op_func;
    op_funcs[2] = ici_o_onerror.op_func;
    op_funcs[3] = ici_o_return.op_func;
    op_funcs[4] = ici_o_mkptr.op_func;
    op_funcs[5] = ici_o_openptr.op_func;
    op_funcs[6] = ici_o_fetch.op_func;
    init_op_funcs_table = 0;
}

int ici_archive_op_func_code(int_func *fn)
{
    int i;

    if (init_op_funcs_table)
        init_op_funcs();
    for (i = 0; i < NOPS; ++i)
    {
        if (fn == op_funcs[i])
            return i;
    }
    return -1;
}

int_func *ici_archive_op_func(int code)
{
    if (init_op_funcs_table)
        init_op_funcs();
    if (code < 0 || code >= NOPS)
    {
        return NULL;
    }
    return op_funcs[code];
}

void ici_archive_byteswap(void *ptr, int sz)
{
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

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ, "save",        ici_archive_f_save,     0, 0},
    {ICI_CF_OBJ, "restore",     ici_archive_f_restore,  0, 0},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_serialisation_library_init(void)
{
    if
    (
        ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "serialisation")
        ||
        init_ici_str()
        ||
        (ici_archive_tcode = ici_register_type(&ici_archive_type)) == 0
    )
    {
        return NULL;
    }
    return ici_objof(ici_module_new(cfuncs));
}

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
#include "int.h"
#include "null.h"
#include "cfunc.h"
#include "file.h"
#include "struct.h"
#include "op.h"

#include <netinet/in.h>

namespace ici
{

#ifndef htonll
long long htonll(long long v)
{
  assert(sizeof (long long) == 8);
  uint32_t msw = v >> 32ull;
  uint32_t lsw = v & ((1ull<<32)-1);
  return (long long)htonl(msw) | ((long long)htonl(lsw) << 32ull);
}
#endif

typedef int int_func();
static int_func *op_funcs[7];

#define num_op_funcs ((int)(sizeof op_funcs / sizeof op_funcs[0]))

unsigned long archive_type::mark(ici_obj_t *o) {
    o->o_flags |= ICI_O_MARK;
    auto ar = archive_of(o);
    return size + ici_mark(ar->a_file) + ici_mark(ar->a_sent) + ici_mark(ar->a_scope);
}

int ici_archive_init()
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

void ici_archive_uninit()
{
    ici_uninit_saver_map();
    ici_uninit_restorer_map();
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
	ar->a_sent->decref();
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
    return ici_int_new((long)obj);
}

int
ici_archive_insert(ici_archive_t *ar, ici_obj_t *key, ici_obj_t *val)
{
    ici_obj_t *k;
    int failed = 1;

    if ((k = make_key(key)) != NULL)
    {
        failed = ici_assign(ar->a_sent, k, val);
        k->decref();
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
        k->decref();
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
        k->decref();
    }
    return v == ici_null ? NULL : v;
}

void
ici_archive_stop(ici_archive_t *ar)
{
    ar->decref();
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
        long long *ll = (long long *)ptr;
        *ll = htonll(*ll);
        return;
    }

    char *s = (char *)ptr;
    char *e = s + sz - 1;
    int i;

    for (i = 0; i < sz / 2 ; ++i)
    {
        char t = s[i];
        s[i] = e[-i];
        e[-i] = t;
    }
}

ICI_DEFINE_CFUNCS(save_restore)
{
    ICI_DEFINE_CFUNC(save, ici_archive_f_save),
    ICI_DEFINE_CFUNC(restore, ici_archive_f_restore),
    ICI_CFUNCS_END
};

} // namespace ici

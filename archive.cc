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

int init_restorer_map();
void uninit_restorer_map();
int init_saver_map();
void uninit_saver_map();

long long ici_htonll(long long v)
{
    assert(sizeof (long long) == 8);
    uint32_t msw = v >> 32ull;
    uint32_t lsw = v & ((1ull<<32)-1);
    return (long long)htonl(msw) | ((long long)htonl(lsw) << 32ull);
}

long long ici_ntohll(long long v)
{
    return ici_htonll(v);
}

typedef int int_func();
static int_func *op_funcs[7];
constexpr auto num_op_funcs = nels(op_funcs);

size_t archive_type::mark(object *o)
{
    auto ar = archive_of(o);
    return setmark(ar) + ici_mark(ar->a_file) + ici_mark(ar->a_sent) + ici_mark(ar->a_scope);
}

int archive_init()
{
    op_funcs[0] = NULL;
    op_funcs[1] = o_mklvalue.op_func;
    op_funcs[2] = o_onerror.op_func;
    op_funcs[3] = o_return.op_func;
    op_funcs[4] = o_mkptr.op_func;
    op_funcs[5] = o_openptr.op_func;
    op_funcs[6] = o_fetch.op_func;
    if (init_saver_map())
    {
        return 1;
    }
    return init_restorer_map();
}

void archive_uninit()
{
    uninit_saver_map();
    uninit_restorer_map();
}

archive *archive::start(file *f, objwsup *scope)
{
    archive *ar = ici_talloc(archive);
    if (ar != NULL)
    {
        ICI_OBJ_SET_TFNZ(ar, ICI_TC_ARCHIVE, 0, 1, 0);
        if ((ar->a_sent = ici_struct_new()) == NULL)
        {
            ici_tfree(ar, archive);
            return NULL;
        }
	    ar->a_sent->decref();
        ar->a_file = f;
	    ar->a_scope = scope;
        ici_rego(ar);
    }
    return ar;
}

inline object *make_key(object *obj)
{
    return ici_int_new((int64_t)obj);
}

int archive::insert(object *key, object *val)
{
    int rc = 1;
    if (auto k = make_key(key))
    {
        rc = a_sent->assign(k, val);
        k->decref();
    }
    return rc;
}

void archive::uninsert(object *key)
{
    if (auto k = make_key(key))
    {
        unassign(a_sent, k);
        k->decref();
    }
}

object *archive::lookup(object *obj)
{
    object *v = ici_null;
    if (auto k = make_key(obj))
    {
        v = a_sent->fetch(k);
        k->decref();
    }
    return v == ici_null ? NULL : v;
}

void archive::stop()
{
    decref();
}

int archive_op_func_code(int_func *fn)
{
    for (size_t i = 0; i < num_op_funcs; ++i)
    {
        if (fn == op_funcs[i])
            return i;
    }
    return -1;
}

int_func *archive_op_func(int code)
{
    if (code < 0 || size_t(code) >= num_op_funcs)
    {
        return NULL;
    }
    return op_funcs[code];
}

void archive_byteswap(void *ptr, int sz)
{
    if (sz == sizeof (long long))
    {
        long long *ll = (long long *)ptr;
        *ll = ici_htonll(*ll);
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
    ICI_DEFINE_CFUNC(save, f_archive_save),
    ICI_DEFINE_CFUNC(restore, f_archive_restore),
    ICI_CFUNCS_END()
};

} // namespace ici

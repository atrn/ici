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
#include "archiver.h"
#include "int.h"
#include "null.h"
#include "cfunc.h"
#include "file.h"
#include "map.h"
#include "op.h"

#include <netinet/in.h>

namespace ici
{

inline long long ici_htonll(int64_t v) {
    // archiver::byteswap(&v, sizeof v);
    return v;
}

inline long long ici_ntohll(int64_t v) {
    // archiver::byteswap(&v, sizeof v);
    return v;
}

typedef int int_func();
static int_func *op_funcs[7];
constexpr auto num_op_funcs = nels(op_funcs);

int archive_init() {
    op_funcs[0] = nullptr;
    op_funcs[1] = o_mklvalue.op_func;
    op_funcs[2] = o_onerror.op_func;
    op_funcs[3] = o_return.op_func;
    op_funcs[4] = o_mkptr.op_func;
    op_funcs[5] = o_openptr.op_func;
    op_funcs[6] = o_fetch.op_func;
    return 0;
}

void archive_uninit() {
}

inline object *make_key(object *obj) {
    return new_int((int64_t)obj);
}

archiver::archiver(file *f, objwsup *scope)
    : a_file(f)
    , a_sent(new_map())
    , a_scope(scope)
{
}

archiver::~archiver() {
    if (a_sent) {
        a_sent->decref();
    }
}

int archiver::record(object *obj, object *ref) {
    if (auto k = make_key(obj)) {
        if (a_sent->assign(k, ref)) {
            k->decref();
            return 1;
        }
        k->decref();
        return 0;
    }
    return 1;
}

void archiver::remove(object *obj) {
    if (auto k = make_key(obj)) {
        unassign(a_sent, k);
        k->decref();
    }
}

int archiver::save_name(object *o) {
    const int64_t ref = (int64_t)o;
    if (record(o, o)) {
        return 1;
    }
    if (write(ref)) {
        return 1;
    }
    return 0;
}

int archiver::restore_name(object **name) {
    int64_t ref;
    if (read(&ref)) {
        return 1;
    }
    *name = (object *)ref;
    return 0;
}

int archiver::save_ref(object *o) {
    const uint8_t tcode = TC_REF;
    const int64_t ref = (int64_t)o;
    if (write(tcode)) {
        return 1;
    }
    if (write(ref)) {
        return 1;
    }
    return 0;
}

object *archiver::restore_ref() {
    object *name;
    if (restore_name(&name)) {
        return nullptr;
    }
    if (auto o = lookup(name)) {
        return ici_copy(o);
    }
    return nullptr;
}

object *archiver::lookup(object *obj) {
    object *v = null;
    if (auto k = make_key(obj)) {
        v = a_sent->fetch(k);
        k->decref();
    }
    return v == null ? nullptr : v;
}

int archiver::op_func_code(int_func *fn)
{
    for (size_t i = 0; i < num_op_funcs; ++i) {
        if (fn == op_funcs[i]) {
            return int(i);
        }
    }
    return -1;
}

int_func *archiver::op_func(int code) {
    if (code < 0 || size_t(code) >= num_op_funcs) {
        return nullptr;
    }
    return op_funcs[code];
}

void archiver::byteswap(void *ptr, int sz) {
#if 1 // ICI_ARCHIVE_LITTLE_ENDIAN_HOST
    (void)ptr;
    (void)sz;
#else
    char *s = (char *)ptr;
    char *e = s + sz - 1;
    int i;
    for (i = 0; i < sz / 2 ; ++i) {
        char t = s[i];
        s[i] = e[-i];
        e[-i] = t;
    }
#endif
}

int archiver::read(void *buf, int len) {
    return a_file->read(buf, len) != len;
}

int archiver::read(int16_t *hword) {
    int16_t tmp;
    if (read(&tmp, sizeof tmp)) {
    	return 1;
    }
    // *hword = ntohs(tmp);
    *hword = tmp;
    return 0;
}

int archiver::read(int32_t *aword) {
    int32_t tmp;
    if (read(&tmp, sizeof tmp)) {
    	return 1;
    }
    // *aword = ntohl(tmp);
    *aword = tmp;
    return 0;
}

int archiver::read(int64_t *dword) {
    int64_t tmp;
    if (read(&tmp, sizeof tmp)) {
    	return 1;
    }
    // *dword = ici_ntohll(tmp);
    *dword = tmp;
    return 0;
}

int archiver::read(double *dbl) {
    if (read(dbl, sizeof *dbl)) {
        return 1;
    }
    // byteswap(&dbl, sizeof dbl);
    return 0;
}

int archiver::write(const void *p, int n) {
    return a_file->write(p, n) != n;
}

int archiver::write(int16_t hword) {
    // byteswap(&hword, sizeof hword);
    return write(&hword, sizeof hword);
}

int archiver::write(int32_t aword) {
    // byteswap(&aword, sizeof aword);
    return write(&aword, sizeof aword);
}

int archiver::write(int64_t dword) {
    // byteswap(&dword, sizeof dword);
    return write(&dword, sizeof dword);
}

int archiver::write(double v) {
    // byteswap(&v, sizeof v);
    return write(&v, sizeof v);
}

int archiver::save(object *o) {
    if (auto p = lookup(o)) { // already sent in this session
        return save_ref(p);
    }
    uint8_t tcode = o->o_tcode & 0x1F; // mask out user-bits
    if (o->isatom()) {
        tcode |= O_ARCHIVE_ATOMIC;
    }
    if (write(tcode)) {
        return 1;
    }
    if (o->icitype()->save(this, o)) {
        return 1;
    }
    return 0;
}

object *archiver::restore() {
    uint8_t tcode;
    uint8_t flags = 0;
    if (read(&tcode)) {
    	return nullptr;
    }
    if (tcode == TC_REF) {
        return restore_ref();
    }
    if ((tcode & O_ARCHIVE_ATOMIC) != 0) {
        flags |= object::O_ATOM;
        tcode &= ~O_ARCHIVE_ATOMIC;
    }
    auto t = types[tcode];
    if (!t) {
        set_error("no type with code %02X", tcode);
        return nullptr;
    }
    auto o = t->restore(this);
    if (!o) {
        return nullptr;
    }
    if (flags & object::O_ATOM) {
        o = atom(o, 1);
    }
    return o;
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
int f_archive_save(...) {
    objwsup *scp = mapof(vs.a_top[-1])->o_super;
    file *file;
    object *obj;
    int failed = 1;

    switch (NARGS()) {
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
        if ((file = need_stdout()) == nullptr)
            return 1;
        break;

    default:
        return argerror(2);
    }

    archiver ar(file, scp);
    if (ar) {
        failed = ar.save(obj);
    }
    return failed ? failed : null_ret();
}

int f_archive_restore(...) {
    file *file;
    objwsup *scp;
    object *obj = nullptr;

    scp = mapof(vs.a_top[-1])->o_super;
    switch (NARGS()) {
    case 0:
        if ((file = need_stdin()) == nullptr) {
            return 1;
        }
	break;

    case 1:
        if (typecheck("u", &file)) {
            if (typecheck("d", &scp)) {
		return 1;
            }
	    if ((file = need_stdin()) == nullptr) {
		return 1;
            }
	}
	break;

    default:
	if (typecheck("ud", &file, &scp)) {
	    return 1;
        }
	break;
    }

    {
        archiver ar(file, scp);
        if (ar) {
            obj = ar.restore();
        }
    }

    return obj == nullptr ? 1 : ret_with_decref(obj);
}

ICI_DEFINE_CFUNCS(save_restore)
{
    ICI_DEFINE_CFUNC(save, f_archive_save),
    ICI_DEFINE_CFUNC(restore, f_archive_restore),
    ICI_CFUNCS_END()
};

} // namespace ici

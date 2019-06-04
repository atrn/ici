#define ICI_CORE

/*
 * Object Serialization
 *
 * The ICI 'archiver' serializes ICI object graphs, reading and
 * writing a binary encoded representation of the ICI objects using a
 * format known as the ICI object serialization protocol.
 *
 * All builtin object types, apart from files, may be saved and
 * subsequently restored, this includes function objects.
 *
 * The two functions save and restore write and read objects. Both
 * functions may be passed an ICI file object used as the source or
 * destination of object data. The functions default to using the
 * standard input and output files as per other ICI I/O functions.
 *
 * Serialization Protocol
 *
 * The ICI object serialization protocol is a tagged binary protocol.
 * The protocol is platform independent in that it defines the size
 * and format of all exchanged data and conforming implementations
 * must adhere to those sizes and formats.
 *
 * All integral types are represented in network byte order, floating
 * pointed is defined to be 64-bit IEEE-754 double precision values,
 * byte swapped in the same manner as 64-bit integers. Object sizes
 * and lengths are sent as 32-bit unsigned values.
 *
 * The exact protocol follows.
 *
 * tcode        byte, top-bit set if object is an ICI atom
 *
 *      A tcode encodes two values. A type code in the range
 *      [0, 127] and a flag indicating the value is an ICI
 *      atom, an atomic, read-only value.
 *
 *
 * object ::- tcode <tcode-specific-data> ;
 *
 *      An object is sent as a _tcode_ (type code) followed by zero
 *      or more bytes of data specific to that type.
 *
 * null ::- tcode
 * int ::- tcode <int64>
 * float ::- tcode <float64>
 * string ::- tcode <length> [<byte>...]
 * regexp ::- tcode <options> <length> [<byte>...]
 * array ::- tcode <length> [<object>...]
 * set ::- tcode <length> [<object>...]
 * map ::- tcode <length> [ <key> <value> ]...
 * mem ::- tcode <accessz> <length> <byte>...
 *
 * op ::- tcode
 * func ::- tcode
 * cfunc ::- tcode
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
static void swapout(void *ptr, int sz) {
#if defined(ICI_ARCHIVE_LITTLE_ENDIAN_HOST)
    switch (sz) {
    case 2:
        {
            int16_t *v = (int16_t *)ptr;
            auto tmp = htons(*v);
            *v = tmp;
        }
        break;
    case 4:
        {
            int32_t *v = (int32_t *)ptr;
            auto tmp = htonl(*v);
            *v = tmp;
        }
        break;
    case 8:
        {
            int64_t *v = (int64_t *)ptr;
            auto tmp = htonll(*v);
            *v = tmp;
        }
        break;
    default:
        abort();
    }
#else
    (void)ptr;
    (void)sz;
#endif
}

static void swapin(void *ptr, int sz) {
#if defined(ICI_ARCHIVE_LITTLE_ENDIAN_HOST)
    switch (sz) {
    case 2:
        {
            int16_t *v = (int16_t *)ptr;
            auto tmp = ntohs(*v);
            *v = tmp;
        }
        break;
    case 4:
        {
            int32_t *v = (int32_t *)ptr;
            auto tmp = ntohl(*v);
            *v = tmp;
        }
        break;
    case 8:
        {
            int64_t *v = (int64_t *)ptr;
            auto tmp = ntohll(*v);
            *v = tmp;
        }
        break;
    default:
        abort();
    }
#else
    (void)ptr;
    (void)sz;
#endif
}

#ifdef BINOPFUNC
constexpr auto num_op_funcs = 13;
#else
constexpr auto num_op_funcs = 12;
#endif

typedef int int_func();
static int_func *op_funcs[num_op_funcs];

int archive_init() {
    op_funcs[0] = nullptr;
    op_funcs[1] = o_mklvalue.op_func;
    op_funcs[2] = o_onerror.op_func;
    op_funcs[3] = o_return.op_func;
    op_funcs[4] = o_mkptr.op_func;
    op_funcs[5] = o_openptr.op_func;
    op_funcs[6] = o_fetch.op_func;
    op_funcs[7] = op_unary;
    op_funcs[8] = op_forall;
    op_funcs[9] = op_for;
    op_funcs[10] = op_onerror;
    op_funcs[11] = op_return;
#ifdef BINOPFUNC
    op_funcs[12] = op_binop;
#endif
    return 0;
}

void archive_uninit() {
}

inline ref<integer> make_key(object *obj) {
    return make_ref(new_int((int64_t)obj));
}

archiver::archiver(file *f, objwsup *scope)
    : a_file(f)
    , a_scope(scope)
    , a_sent(new_map())
    , a_names(new_array())
{
}

archiver::operator bool() const {
    return a_sent != nullptr && a_names != nullptr;
}

archiver::~archiver() {
}

int archiver::push_name(str *name) {
    return a_names->push_back(name);
}

int archiver::pop_name() {
    a_names->pop_back();
    return 0;
}

int archiver::record(object *obj, object *ref) {
    if (auto k = make_key(obj)) {
        return a_sent->assign(k, ref);
    }
    return 1;
}

void archiver::remove(object *obj) {
    if (auto k = make_key(obj)) {
        unassign(a_sent, k);
    }
}

int archiver::save_name(object *o) {
    const int64_t value = (int64_t)o;
    if (record(o, o)) {
        return 1;
    }
    if (write(value)) {
        return 1;
    }
    return 0;
}

int archiver::restore_name(object **name) {
    int64_t value;
    if (read(&value)) {
        return 1;
    }
    *name = (object *)value;
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
        incref(o);
        return o;
    }
    return nullptr;
}

object *archiver::lookup(object *obj) {
    object *v = nullptr;
    if (auto k = make_key(obj)) {
        v = a_sent->fetch(k);
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

int archiver::read(int16_t *hword) {
    if (read(hword, sizeof *hword)) {
        return 1;
    }
    swapin(hword, sizeof *hword);
    return 0;
}

int archiver::read(int32_t *aword) {
    if (read(aword, sizeof *aword)) {
        return 1;
    }
    swapin(aword, sizeof *aword);
    return 0;
}

int archiver::read(int64_t *dword) {
    if (read(dword, sizeof *dword)) {
        return 1;
    }
    swapin(dword, sizeof *dword);
    return 0;
}

int archiver::read(float *flt) {
    if (read(flt, sizeof *flt)) {
        return 1;
    }
    swapin(flt, sizeof *flt);
    return 0;
}

int archiver::read(double *dbl) {
    if (read(dbl, sizeof *dbl)) {
        return 1;
    }
    swapin(dbl, sizeof *dbl);
    return 0;
}

int archiver::write(int16_t hword) {
    swapout(&hword, sizeof hword);
    return write(&hword, sizeof hword);
}

int archiver::write(int32_t aword) {
    swapout(&aword, sizeof aword);
    return write(&aword, sizeof aword);
}

int archiver::write(int64_t dword) {
    swapout(&dword, sizeof dword);
    return write(&dword, sizeof dword);
}

int archiver::write(float v) {
    swapout(&v, sizeof v);
    return write(&v, sizeof v);
}

int archiver::write(double v) {
    swapout(&v, sizeof v);
    return write(&v, sizeof v);
}

int archiver::save(object *o) {
    if (auto p = lookup(o)) { // if already sent in this session
        return save_ref(p);   // save a reference to the object
    }
    uint8_t tcode = o->o_tcode & object::O_ICIBITS; // mask out user-bits
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
    if (tcode >= num_types) {
        set_error("restored type code %d exceeds number of registered types %d", (int)tcode, num_types);
        return nullptr;
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
 * save(any, [file [, map]])
 *
 * Save an object to a file by writing a serialized object graph
 * using the given object as the root of the graph.
 *
 * If file is not given the object is written to the standard output.
 *
 * If map is supplied it is used as the 'scope' during the
 * save operation.
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
        if (typecheck("oud", &obj, &file, &scp))
            return 1;
        break;

    case 2:
        if (typecheck("ou", &obj, &file))
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

str *archiver::name_qualifier() {

    if (a_names->len() == 0) {
        return stringof(SS(empty_string));
    }

    auto e = a_names->astart();
    int len = stringof(*e)->s_nchars;

    for (e = a_names->anext(e); e != a_names->alimit(); e = a_names->anext(e)) {
        len = len + 1 /* '.' (dot) */ + stringof(*e)->s_nchars;
    }
    auto s = str_alloc(len);
    if (!s) {
        return nullptr;
    }
    auto p = s->s_chars;
    for (e = a_names->astart(); e != a_names->alimit(); e = a_names->anext(e)) {
        auto s = stringof(*e);
        memcpy(p, s->s_chars, s->s_nchars);
        p += s->s_nchars;
        *p++ = '.';
    }
    return str_intern(s);
}

} // namespace ici

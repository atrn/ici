#define ICI_CORE
#include "vec.h"
#include "int.h"
#include "float.h"
#include "forall.h"
#include "map.h"
#include "null.h"
#include "str.h"
#include "archiver.h"

namespace ici
{

template struct vec<TC_VEC32, float>;
template struct vec<TC_VEC64, double>;

namespace
{

template <typename vec> vec *new_vec(size_t nvec, size_t count, object *props)
{
    auto f = ici_talloc<vec>();
    if (!f)
    {
        return nullptr;
    }
    if (props)
    {
        assert(ismap(props));
        f->_props = mapof(props);
    }
    else
    {
        f->_props = new_map();
        if (!f->_props)
        {
            ici_free(f);
            return nullptr;
        }
    }
    f->_ptr = static_cast<typename vec::value_type *>(ici_alloc(nvec * sizeof (typename vec::value_type)));
    if (!f->_ptr)
    {
        ici_free(f);
        return nullptr;
    }
    f->set_tfnz(vec::type_code, 0, 1, 0);
    f->_size = nvec;
    f->_count = count;
    rego(f);
    return f;
}

template <typename vec> object *dofetch(vec *f, object *k)
{
    if (isint(k))
    {
        auto ofs = intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->_count)
        {
            set_error("index out of range");
            return nullptr;
        }
        return new_float((*f)[ofs]);
    }

    if (k == SS(count))
    {
        return new_int(f->_count);
    }

    if (k == SS(size))
    {
        return new_int(f->_size);
    }

    return f->_props->fetch(k);
}

template <typename vec> int doassign(vec *f, object *k, object *v)
{
    if (isint(k))
    {
        auto ofs = intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->_size)
        {
            return set_error("index out of range");
        }
        for (; f->_count < static_cast<size_t>(ofs); ++f->_count)
        {
            (*f)[f->_count] = 0.0f;
        }
        if (isint(v))
        {
            (*f)[ofs] = static_cast<typename vec::value_type>(intof(v)->i_value);
            ++f->_count;
            return 0;
        }
        if (isfloat(v))
        {
            (*f)[ofs] = static_cast<typename vec::value_type>(floatof(v)->f_value);
            ++f->_count;
            return 0;
        }
        return type::assign_fail(f, k, v);
    }

    if (k == SS(size))
    {
        return type::assign_fail(f, k, v);
    }

    if (k == SS(count))
    {
        if (isint(v))
        {
            size_t count = intof(v)->i_value;
            if (intof(v)->i_value < 0 || count > f->_size)
            {
                return set_error("%u: count out of range", count);
            }
            f->_count = count;
            return 0;
        }
    }

    return f->_props->assign(k, v);
}

template <typename vec> int doforall(object *o)
{
    auto     fa = forallof(o);

    auto f = static_cast<vec *>(fa->fa_aggr);
    if (++fa->fa_index >= f->_count) {
        return -1;
    }
    if (fa->fa_vaggr != null) {
        auto v = make_ref(new_float(f->_ptr[fa->fa_index]));
        if (!v)
            return 1;
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, v)) {
            return 1;
        }
    }
    if (fa->fa_kaggr != null) {
        integer *i;
        if ((i = make_ref(new_int((long)fa->fa_index))) == nullptr) {
            return 1;
        }
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i)) {
            return 1;
        }
    }
    return 0;
}

template <typename vec> int dosave(archiver *ar, vec *f)
{
    if (ar->save_name(f)) {
        return 1;
    }
    int64_t size = f->_size;
    if (ar->write(size)) {
        return 1;
    }
    int64_t count = f->_count;
    if (ar->write(count)) {
        return 1;
    }
    if (ar->save(f->_props)) {
        return 1;
    }
    for (size_t i = 0; i < f->_count; ++i) {
        if (ar->write(f->_ptr[i])) {
            return 1;
        }
    }
    return 0;
}

template <typename vec> object *dorestore(archiver *ar)
{
    object *oname;
    int64_t size;
    int64_t count;
    object *props;

    if (ar->restore_name(&oname)) {
        return nullptr;
    }
    if (ar->read(&size)) {
        return nullptr;
    }
    if (ar->read(&count)) {
        return nullptr;
    }
    if ((props = ar->restore()) == nullptr) {
        ar->remove(oname);
        return nullptr;
    }
    if (!ismap(props)) {
        set_error("restored properties is not a map");
        return nullptr;
    }
    if (count > size) {
        set_error("count greater than size");
        return nullptr;
    }

    auto f = make_ref<vec>(new_vec<vec>(size, count, props));
    if (ar->record(oname, f)) {
        return nullptr;
    }

    for (size_t i = 0; i < f->_count; ++i)
    {
        if (ar->read(&f->_ptr[i]))
        {
            return nullptr;
        }
    }
    return f;
}

} // anon



//  ----------------------------------------------------------------

size_t vec32_type::mark(object *o)
{
    return type::mark(o)
        + vec32of(o)->_props->mark()
        + vec32of(o)->_size * sizeof (vec32::value_type);
}

void vec32_type::free(object *o)
{
    ici_free(vec32of(o)->_ptr);
    type::free(o);
}

int64_t vec32_type::len(object *o)
{
    return vec32of(o)->_count;
}

object *vec32_type::fetch(object *o, object *k)
{
    return dofetch(vec32of(o), k);
}

int vec32_type::assign(object *o, object *k, object *v)
{
    return doassign(vec32of(o), k, v);
}

int vec32_type::forall(object *o)
{
    return doforall<vec32>(o);
}

int vec32_type::save(archiver *ar, object *o)
{
    return dosave(ar, vec32of(o));
}

object * vec32_type::restore(archiver *ar)
{
    return dorestore<vec32>(ar);
}

vec32 *new_vec32(size_t size, size_t count, object *props)
{
    return new_vec<vec32>(size, count, props);
}

//  ----------------------------------------------------------------

size_t vec64_type::mark(object *o)
{
    return type::mark(o)
        + vec64of(o)->_props->mark()
        + vec64of(o)->_size * sizeof(vec64::value_type);
}

void vec64_type::free(object *o)
{
    ici_free(vec64of(o)->_ptr);
    type::free(o);
}

int64_t vec64_type::len(object *o)
{
    return vec64of(o)->_count;
}

object *vec64_type::fetch(object *o, object *k)
{
    return dofetch(vec64of(o), k);
}

int vec64_type::assign(object *o, object *k, object *v)
{
    return doassign(vec64of(o), k, v);
}

int vec64_type::forall(object *o)
{
    return doforall<vec64>(o);
}

int vec64_type::save(archiver *ar, object *o)
{
    return dosave(ar, vec64of(o));
}

object * vec64_type::restore(archiver *ar)
{
    return dorestore<vec64>(ar);
}

vec64 *new_vec64(size_t size, size_t count, object *props)
{
    return new_vec<vec64>(size, count, props);
}

} // namespace ici

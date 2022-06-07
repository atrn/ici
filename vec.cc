#define ICI_CORE
#include "vec.h"
#include "archiver.h"
#include "error.h"
#include "float.h"
#include "forall.h"
#include "int.h"
#include "map.h"
#include "null.h"
#include "str.h"

#ifdef ICI_VEC_USE_IPP
#include <ippcore.h>
#include <ipps.h>
#endif

// When uing IPP we allocate the memory for a vec's array using the
// ipp malloc function to ensure correct alignment and anything else
// IPP may do. In this case we do NOT account for that memory when
// returning a vec's size during the GC mark phase as its not really
// ICI memory in use.
//
// The following, inline, functions hide the details.
//
// - allocvec   Allocate N values for a vec's array.
// - freevec    Free memory allocated for a vec's array.
// - vecmemuse  Return the GC size of the vec array.
//
#ifdef ICI_VEC_USE_IPP
template <typename vec_type> inline typename vec_type::value_type *allocvec(size_t z)
{
    using value_type = typename vec_type::value_type;
    return static_cast<value_type *>(ippMalloc(z * sizeof(value_type)));
}

template <typename vec_type> inline void freevec(vec_type *vec)
{
    ippFree(vec->v_ptr);
}

template <typename vec_type> inline size_t vecmemuse(vec_type *)
{
    return 0;
}
#else
template <typename vec_type> inline typename vec_type::value_type *allocvec(size_t z)
{
    using value_type = typename vec_type::value_type;
    return static_cast<value_type *>(ici::ici_alloc(z * sizeof(value_type)));
}

template <typename vec_type> inline void freevec(vec_type *vec)
{
    ici::ici_free(vec->v_ptr);
}

template <typename vec_type> inline size_t vecmemuse(vec_type *vec)
{
    using value_type = typename vec_type::value_type;
    return vec->v_capacity * sizeof(value_type);
}
#endif

namespace ici
{

template struct vec<TC_VEC32, float>;
template struct vec<TC_VEC64, double>;

#ifdef ICI_VEC_USE_IPP
template <> void vec<TC_VEC32, float>::fill(float value, size_t ofs, size_t lim)
{
    ippsSet_32f(value, &v_ptr[ofs], lim - ofs);
    v_size = lim;
}

template <> void vec<TC_VEC32, float>::fill(float value, size_t ofs)
{
    fill(value, ofs, v_capacity);
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator=(float value)
{
    fill(value);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator+=(const vec &rhs)
{
    ippsAdd_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator-=(const vec &rhs)
{
    ippsSub_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator*=(const vec &rhs)
{
    ippsMul_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator/=(const vec &rhs)
{
    ippsDiv_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator+=(value_type value)
{
    ippsAddC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator-=(value_type value)
{
    ippsSubC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator*=(value_type value)
{
    ippsMulC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC32, float> &vec<TC_VEC32, float>::operator/=(value_type value)
{
    ippsDivC_32f_I(value, v_ptr, v_size);
    return *this;
}

// TC_VEC64 -------------------------------------------------------

template <> void vec<TC_VEC64, double>::fill(double value, size_t ofs, size_t lim)
{
    ippsSet_64f(value, &v_ptr[ofs], lim - ofs);
    v_size = lim;
}

template <> void vec<TC_VEC64, double>::fill(double value, size_t ofs)
{
    fill(value, ofs, v_capacity);
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator=(double value)
{
    fill(value);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator+=(const vec &rhs)
{
    ippsAdd_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator-=(const vec &rhs)
{
    ippsSub_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator*=(const vec &rhs)
{
    ippsMul_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator/=(const vec &rhs)
{
    ippsDiv_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator+=(value_type value)
{
    ippsAddC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator-=(value_type value)
{
    ippsSubC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator*=(value_type value)
{
    ippsMulC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <> vec<TC_VEC64, double> &vec<TC_VEC64, double>::operator/=(value_type value)
{
    ippsDivC_64f_I(value, v_ptr, v_size);
    return *this;
}

#endif // ICI_VEC_USE_IPP

namespace
{

int index_error(int64_t ofs)
{
    return set_error("%lld: index out of range", ofs);
}

// ----------------------------------------------------------------

//  Create a vec of the given capacity, initial size and properties map
//
template <typename vec_type> vec_type *new_vec(size_t capacity, size_t size, object *props)
{
    auto v = ici_talloc<vec_type>();
    if (!v)
    {
        return nullptr;
    }
    if (props)
    {
        assert(ismap(props));
        v->v_props = mapof(props);
    }
    else
    {
        v->v_props = new_map();
        if (!v->v_props)
        {
            ici_free(v);
            return nullptr;
        }
    }
    v->v_ptr = allocvec<vec_type>(capacity);
    if (!v->v_ptr)
    {
        ici_free(v);
        return nullptr;
    }
    v->set_tfnz(vec_type::type_code, 0, 1, 0);
    v->v_capacity = capacity;
    v->v_size = size;
    v->v_parent = nullptr;
    rego(v);
    return v;
}

// Create a vec that is a copy of some other vec.
//
template <typename vec_type> vec_type *new_vec(vec_type *other)
{
    using value_type = typename vec_type::value_type;

    auto v = ici_talloc<vec_type>();
    if (!v)
    {
        return nullptr;
    }
    v->set_tfnz(vec_type::type_code, 0, 1, 0);
    v->v_capacity = other->v_capacity;
    v->v_size = other->v_size;
    v->v_parent = nullptr;
    v->v_props = mapof(copyof(other->v_props));
    if (!v->v_props)
    {
        ici_free(v);
        return nullptr;
    }
    decref(v->v_props);
    v->v_ptr = allocvec<vec_type>(other->v_capacity);
    if (!v->v_ptr)
    {
        ici_free(v);
        return nullptr;
    }
    const auto nbytes = other->v_capacity * sizeof(value_type);
    memcpy(v->v_ptr, other->v_ptr, nbytes);
    rego(v);
    return v;
}

//  Create a new vec by copying another, converting its values one-by-one.
//
template <typename vec_type, typename other_type> vec_type *new_vec_conv(other_type *other)
{
    using value_type = typename vec_type::value_type;

    auto v = ici_talloc<vec_type>();
    if (!v)
    {
        return nullptr;
    }
    v->set_tfnz(vec_type::type_code, 0, 1, 0);
    v->v_capacity = other->v_capacity;
    v->v_size = other->v_size;
    v->v_parent = nullptr;
    v->v_props = mapof(copyof(other->v_props));
    if (!v->v_props)
    {
        ici_free(v);
        return nullptr;
    }
    decref(v->v_props);
    v->v_ptr = allocvec<vec_type>(other->v_capacity);
    if (!v->v_ptr)
    {
        ici_free(v);
        return nullptr;
    }
    for (size_t i = 0; i < other->v_capacity; ++i)
    {
        v->v_ptr[i] = value_type(other->v_ptr[i]);
    }
    rego(v);
    return v;
}

//  Creates a new vec by taking a slice of another vec.  The resultant
//  vec shares the arguments underlying data and does not free it.
//
template <typename vec_type, typename other_type> vec_type *new_vec(other_type *other, size_t offset, size_t len)
{
    if (offset + len >= other->v_size)
    {
        set_error("slice bounds exceed parent's size");
        return nullptr;
    }
    auto v = ici_talloc<vec_type>();
    if (!v)
    {
        return nullptr;
    }
    v->set_tfnz(vec_type::type_code, 0, 1, 0);
    v->v_capacity = other->v_capacity - len;
    v->v_ptr = other->v_ptr + offset;
    v->v_size = len;
    v->v_props = mapof(copyof(other->v_props));
    v->v_parent = other;
    if (!v->v_props)
    {
        ici_free(v);
        return nullptr;
    }
    decref(v->v_props);
    rego(v);
    return v;
}

template <typename vec_type> object *fetch_vec(vec_type *f, object *k)
{
    if (isint(k))
    {
        auto ofs = intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->v_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->v_size)
        {
            index_error(ofs);
            return nullptr;
        }
        return new_float((*f)[ofs]);
    }

    if (k == SS(size))
    {
        auto o = new_int(f->v_size);
        if (o)
        {
            o->decref();
        }
        return o;
    }

    if (k == SS(capacity))
    {
        auto o = new_int(f->v_capacity);
        if (o)
        {
            o->decref();
        }
        return o;
    }

    return f->v_props->fetch(k);
}

template <typename vec_type> object *copy_vec(vec_type *v)
{
    return new_vec(v);
}

template <typename vec_type> int assign_vec(vec_type *f, object *k, object *v)
{
    using value_type = typename vec_type::value_type;
    constexpr auto zero = value_type(0);

    if (isint(k))
    {
        auto ofs = intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->v_size + ofs;
        }
        if (ofs < 0 || static_cast<size_t>(ofs) >= f->v_capacity)
        {
            return index_error(ofs);
        }
        if (!isint(v) && !isfloat(v))
        {
            return type::assign_fail(f, k, v);
        }
        for (; f->v_size < static_cast<size_t>(ofs); ++f->v_size)
        {
            (*f)[f->v_size] = zero;
        }
        if (isint(v))
        {
            (*f)[ofs] = static_cast<value_type>(intof(v)->i_value);
        }
        else
        {
            (*f)[ofs] = static_cast<value_type>(floatof(v)->f_value);
        }
        ++f->v_size;
        return 0;
    }

    if (k == SS(capacity))
    {
        return type::assign_fail(f, k, v);
    }

    if (k == SS(size))
    {
        if (!isint(v))
        {
            return type::assign_fail(f, k, v);
        }
        const size_t z = intof(v)->i_value;
        if (z > f->v_capacity)
        {
            return set_error("%u: size exceeds capacity", z);
        }
        f->v_size = z;
        return 0;
    }

    return f->v_props->assign(k, v);
}

template <typename vec_type> int forall_vec(object *o)
{
    auto fa = forallof(o);

    auto f = static_cast<vec_type *>(fa->fa_aggr);
    if (++fa->fa_index >= f->v_size)
    {
        return -1;
    }
    if (fa->fa_vaggr != null)
    {
        auto v = make_ref(new_float(f->v_ptr[fa->fa_index]));
        if (!v)
        {
            return 1;
        }
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, v))
        {
            return 1;
        }
    }
    if (fa->fa_kaggr != null)
    {
        integer *i;
        if ((i = make_ref(new_int(int64_t(fa->fa_index)))) == nullptr)
        {
            return 1;
        }
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
        {
            return 1;
        }
    }
    return 0;
}

template <typename vec_type> int save_vec(archiver *ar, vec_type *f)
{
    if (ar->save_name(f))
    {
        return 1;
    }
    int64_t capacity = f->v_capacity;
    if (ar->write(capacity))
    {
        return 1;
    }
    int64_t size = f->v_size;
    if (ar->write(size))
    {
        return 1;
    }
    if (ar->save(f->v_props))
    {
        return 1;
    }
    for (size_t i = 0; i < f->v_size; ++i)
    {
        if (ar->write(f->v_ptr[i]))
        {
            return 1;
        }
    }
    return 0;
}

template <typename vec_type> object *restore_vec(archiver *ar)
{
    object *oname;
    int64_t capacity;
    int64_t size;
    object *props;

    if (ar->restore_name(&oname))
    {
        return nullptr;
    }
    if (ar->read(&capacity))
    {
        return nullptr;
    }
    if (ar->read(&size))
    {
        return nullptr;
    }
    if ((props = ar->restore()) == nullptr)
    {
        ar->remove(oname);
        return nullptr;
    }
    if (!ismap(props))
    {
        set_error("restored properties is not a map");
        return nullptr;
    }
    if (size > capacity)
    {
        set_error("size greater than capacity");
        return nullptr;
    }
    auto f = make_ref(new_vec<vec_type>(capacity, size, props));
    if (ar->record(oname, f))
    {
        return nullptr;
    }
    for (size_t i = 0; i < f->v_size; ++i)
    {
        if (ar->read(&f->v_ptr[i]))
        {
            return nullptr;
        }
    }
    return f;
}

} // namespace

#define DEFINE_VEC_TYPE_CLASS(VECTYPE, VECOF, VECOBJ, OTHERVEC)                                                        \
                                                                                                                       \
    size_t VECTYPE::mark(object *o)                                                                                    \
    {                                                                                                                  \
        return type::mark(o) + VECOF(o)->v_props->mark() + mark_optional(VECOF(o)->v_parent) + vecmemuse(VECOF(o));    \
    }                                                                                                                  \
                                                                                                                       \
    void VECTYPE::free(object *o)                                                                                      \
    {                                                                                                                  \
        if (!VECOF(o)->v_parent)                                                                                       \
        {                                                                                                              \
            freevec(VECOF(o));                                                                                         \
        }                                                                                                              \
        type::free(o);                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    int64_t VECTYPE::len(object *o)                                                                                    \
    {                                                                                                                  \
        return VECOF(o)->v_size;                                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    object *VECTYPE::copy(object *o)                                                                                   \
    {                                                                                                                  \
        return copy_vec(VECOF(o));                                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    object *VECTYPE::fetch(object *o, object *k)                                                                       \
    {                                                                                                                  \
        return fetch_vec(VECOF(o), k);                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    int VECTYPE::assign(object *o, object *k, object *v)                                                               \
    {                                                                                                                  \
        return assign_vec(VECOF(o), k, v);                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    int VECTYPE::forall(object *o)                                                                                     \
    {                                                                                                                  \
        return forall_vec<VECOBJ>(o);                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    int VECTYPE::save(archiver *ar, object *o)                                                                         \
    {                                                                                                                  \
        return save_vec(ar, VECOF(o));                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    object *VECTYPE::restore(archiver *ar)                                                                             \
    {                                                                                                                  \
        return restore_vec<VECOBJ>(ar);                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    VECOBJ *new_##VECOBJ(size_t capacity, size_t size, object *props)                                                  \
    {                                                                                                                  \
        return new_vec<VECOBJ>(capacity, size, props);                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    VECOBJ *new_##VECOBJ(VECOBJ *other)                                                                                \
    {                                                                                                                  \
        return new_vec<VECOBJ>(other);                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    VECOBJ *new_##VECOBJ(OTHERVEC *other)                                                                              \
    {                                                                                                                  \
        return new_vec_conv<VECOBJ>(other);                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    VECOBJ *new_##VECOBJ(VECOBJ *other, size_t offset, size_t len)                                                     \
    {                                                                                                                  \
        return new_vec<VECOBJ>(other, offset, len);                                                                    \
    }

//  ----------------------------------------------------------------

DEFINE_VEC_TYPE_CLASS(vec32_type, vec32of, vec32, vec64)
DEFINE_VEC_TYPE_CLASS(vec64_type, vec64of, vec64, vec32)

// ----------------------------------------------------------------

size_t vec_size(object *o)
{
    if (isvec32(o))
    {
        return vec32of(o)->size();
    }
    if (isvec64(o))
    {
        return vec64of(o)->size();
    }
    set_errorc("attempt to obtain vector size of non-vector");
    return ~0u;
}

} // namespace ici

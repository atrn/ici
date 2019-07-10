#define ICI_CORE
#include "vec.h"
#include "int.h"
#include "float.h"
#include "forall.h"
#include "map.h"
#include "null.h"
#include "str.h"
#include "archiver.h"

#ifdef ICI_VEC_USE_IPP
#include <ippcore.h>
#include <ipps.h>
#endif

namespace ici
{

template struct vec<TC_VEC32, float>;
template struct vec<TC_VEC64, double>;


#ifdef ICI_VEC_USE_IPP

template <>
void
vec<TC_VEC32, float>::fill(float value, size_t ofs, size_t lim)
{
    ippsSet_32f(value, &v_ptr[ofs], lim - ofs);
    v_size = lim;
}

template <>
void
vec<TC_VEC32, float>::fill(float value, size_t ofs)
{
    fill(value, ofs, v_capacity);
}

template <>
vec<TC_VEC32, float> & 
vec<TC_VEC32, float>::operator=(float value)
{
    fill(value);
    return *this;
}

template <>
void
vec<TC_VEC64, double>::fill(double value, size_t ofs, size_t lim)
{
    ippsSet_64f(value, &v_ptr[ofs], lim - ofs);
    v_size = lim;
}

template <>
void
vec<TC_VEC64, double>::fill(double value, size_t ofs)
{
    fill(value, ofs, v_capacity);
}

template <>
vec<TC_VEC64, double> & 
vec<TC_VEC64, double>::operator=(double value)
{
    fill(value);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator+=(const vec &rhs)
{
    ippsAdd_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator-=(const vec &rhs)
{
    ippsSub_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator*=(const vec &rhs)
{
    ippsMul_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator/=(const vec &rhs)
{
    ippsDiv_32f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator+=(value_type value)
{
    ippsAddC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator-=(value_type value)
{
    ippsSubC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator*=(value_type value)
{
    ippsMulC_32f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC32, float> &
vec<TC_VEC32, float>::operator/=(value_type value)
{
    ippsDivC_32f_I(value, v_ptr, v_size);
    return *this;
}

// TC_VEC64 -------------------------------------------------------

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator+=(const vec &rhs)
{
    ippsAdd_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator-=(const vec &rhs)
{
    ippsSub_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator*=(const vec &rhs)
{
    ippsMul_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator/=(const vec &rhs)
{
    ippsDiv_64f_I(rhs.v_ptr, v_ptr, v_size);
    return *this;
}


template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator+=(value_type value)
{
    ippsAddC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator-=(value_type value)
{
    ippsSubC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator*=(value_type value)
{
    ippsMulC_64f_I(value, v_ptr, v_size);
    return *this;
}

template <>
vec<TC_VEC64, double> &
vec<TC_VEC64, double>::operator/=(value_type value)
{
    ippsDivC_64f_I(value, v_ptr, v_size);
    return *this;
}

#endif // ICI_VEC_USE_IPP

namespace
{

static int index_error(int64_t ofs)
{
    return set_error("%lld: index out of range", ofs);
}

template <typename vec_type> vec_type *new_vec(size_t capacity, size_t size, object *props)
{
    using value_type = typename vec_type::value_type;

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
#ifdef ICI_VEC_USE_IPP
    v->v_ptr = static_cast<value_type *>(ippMalloc(capacity * sizeof (value_type)));
#else
    v->v_ptr = static_cast<value_type *>(ici_alloc(capacity * sizeof (value_type)));
#endif
    if (!v->v_ptr)
    {
        ici_free(v);
        return nullptr;
    }
    v->set_tfnz(vec_type::type_code, 0, 1, 0);
    v->v_capacity = capacity;
    v->v_size = size;
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
            ofs = f->v_capacity + ofs;
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

template <typename vec_type> int assign_vec(vec_type *f, object *k, object *v)
{
    using value_type = typename vec_type::value_type;
    constexpr auto zero = value_type(0);

    if (isint(k))
    {
        auto ofs = intof(k)->i_value;
        if (ofs < 0)
        {
            ofs = f->v_capacity + ofs;
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
        if (isint(v))
        {
            size_t z = intof(v)->i_value;
            if (intof(v)->i_value < 0 || z > f->v_capacity)
            {
                return set_error("%u: size out of range", z);
            }
            f->v_size = z;
            return 0;
        }
    }
    return f->v_props->assign(k, v);
}

template <typename vec_type> int forall_vec(object *o)
{
    auto     fa = forallof(o);

    auto f = static_cast<vec_type *>(fa->fa_aggr);
    if (++fa->fa_index >= f->v_size) {
        return -1;
    }
    if (fa->fa_vaggr != null) {
        auto v = make_ref(new_float(f->v_ptr[fa->fa_index]));
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

template <typename vec_type> int save_vec(archiver *ar, vec_type *f)
{
    if (ar->save_name(f)) {
        return 1;
    }
    int64_t capacity = f->v_capacity;
    if (ar->write(capacity)) {
        return 1;
    }
    int64_t size = f->v_size;
    if (ar->write(size)) {
        return 1;
    }
    if (ar->save(f->v_props)) {
        return 1;
    }
    for (size_t i = 0; i < f->v_size; ++i) {
        if (ar->write(f->v_ptr[i])) {
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

    if (ar->restore_name(&oname)) {
        return nullptr;
    }
    if (ar->read(&capacity)) {
        return nullptr;
    }
    if (ar->read(&size)) {
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
    if (size > capacity) {
        set_error("size greater than capacity");
        return nullptr;
    }
    auto f = make_ref(new_vec<vec_type>(capacity, size, props));
    if (ar->record(oname, f)) {
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

} // anon


//  ----------------------------------------------------------------

size_t vec32_type::mark(object *o)
{
    return type::mark(o)
        + vec32of(o)->v_props->mark()
#ifdef ICI_VEC_USE_IPP
    ; // IPP's malloc is a distinct arena
#else
        + vec32of(o)->v_capacity * sizeof (vec32::value_type);
#endif
}

void vec32_type::free(object *o)
{
#ifdef ICI_VEC_USE_IPP
    ippFree(vec32of(o)->v_ptr);
#else
    ici_free(vec32of(o)->v_ptr);
#endif
    type::free(o);
}

int64_t vec32_type::len(object *o)
{
    return vec32of(o)->v_size;
}

object *vec32_type::fetch(object *o, object *k)
{
    return fetch_vec(vec32of(o), k);
}

int vec32_type::assign(object *o, object *k, object *v)
{
    return assign_vec(vec32of(o), k, v);
}

int vec32_type::forall(object *o)
{
    return forall_vec<vec32>(o);
}

int vec32_type::save(archiver *ar, object *o)
{
    return save_vec(ar, vec32of(o));
}

object * vec32_type::restore(archiver *ar)
{
    return restore_vec<vec32>(ar);
}

vec32 *new_vec32(size_t capacity, size_t size, object *props)
{
    return new_vec<vec32>(capacity, size, props);
}

//  ----------------------------------------------------------------

size_t vec64_type::mark(object *o)
{
    return type::mark(o)
        + vec64of(o)->v_props->mark()
        + vec64of(o)->v_capacity * sizeof(vec64::value_type);
}

void vec64_type::free(object *o)
{
    ici_free(vec64of(o)->v_ptr);
    type::free(o);
}

int64_t vec64_type::len(object *o)
{
    return vec64of(o)->v_size;
}

object *vec64_type::fetch(object *o, object *k)
{
    return fetch_vec(vec64of(o), k);
}

int vec64_type::assign(object *o, object *k, object *v)
{
    return assign_vec(vec64of(o), k, v);
}

int vec64_type::forall(object *o)
{
    return forall_vec<vec64>(o);
}

int vec64_type::save(archiver *ar, object *o)
{
    return save_vec(ar, vec64of(o));
}

object * vec64_type::restore(archiver *ar)
{
    return restore_vec<vec64>(ar);
}

vec64 *new_vec64(size_t capacity, size_t size, object *props)
{
    return new_vec<vec64>(capacity, size, props);
}

} // namespace ici

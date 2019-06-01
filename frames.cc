#define ICI_CORE
#include "frames.h"
#include "int.h"
#include "float.h"
#include "forall.h"
#include "map.h"
#include "null.h"
#include "str.h"

namespace ici
{

template struct frames<float>;
template struct frames<double>;

namespace
{

template <typename frame, const int typecode> frame *new_frames(size_t nframes)
{
    auto f = ici_talloc<frame>();
    if (!f)
    {
        return nullptr;
    }
    f->_props = new_map();
    if (!f->_props)
    {
        ici_free(f);
        return nullptr;
    }
    f->_ptr = static_cast<typename frame::data_type *>(ici_alloc(nframes * sizeof (typename frame::data_type)));
    if (!f->_ptr)
    {
        ici_free(f);
        return nullptr;
    }
    f->set_tfnz(typecode, 0, 1, 0);
    f->_size = nframes;
    f->_count = 0;
    rego(f);
    return f;
}

template <typename frame> object *dofetch(frame *f, object *k)
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

template <typename frame> int doassign(frame *f, object *k, object *v)
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
            (*f)[ofs] = static_cast<typename frame::data_type>(intof(v)->i_value);
            ++f->_count;
            return 0;
        }
        if (isfloat(v))
        {
            (*f)[ofs] = static_cast<typename frame::data_type>(floatof(v)->f_value);
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

template <typename frame> int doforall(object *o)
{
    auto     fa = forallof(o);

    auto f = static_cast<frame *>(fa->fa_aggr);
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

} // anon


//  ----------------------------------------------------------------

size_t frames32_type::mark(object *o)
{
    return type::mark(o)
        + frames32of(o)->_props->mark()
        + frames32of(o)->_size * sizeof (frames32::data_type);
}

void frames32_type::free(object *o)
{
    ici_free(frames32of(o)->_ptr);
    type::free(o);
}

int64_t frames32_type::len(object *o)
{
    return frames32of(o)->_count;
}

object *frames32_type::fetch(object *o, object *k)
{
    return dofetch(frames32of(o), k);
}

int frames32_type::assign(object *o, object *k, object *v)
{
    return doassign(frames32of(o), k, v);
}

int frames32_type::forall(object *o)
{
    return doforall<frames32>(o);
}

frames32 *new_frames32(size_t nframes)
{
    return new_frames<frames32, TC_FRAMES32>(nframes);
}

//  ----------------------------------------------------------------

size_t frames64_type::mark(object *o)
{
    return type::mark(o)
        + frames64of(o)->_props->mark()
        + frames64of(o)->_size * sizeof(frames64::data_type);
}

void frames64_type::free(object *o)
{
    ici_free(frames64of(o)->_ptr);
    type::free(o);
}

int64_t frames64_type::len(object *o)
{
    return frames64of(o)->_count;
}

object *frames64_type::fetch(object *o, object *k)
{
    return dofetch(frames64of(o), k);
}

int frames64_type::assign(object *o, object *k, object *v)
{
    return doassign(frames64of(o), k, v);
}

int frames64_type::forall(object *o)
{
    return doforall<frames64>(o);
}

frames64 *new_frames64(size_t nframes)
{
    return new_frames<frames64, TC_FRAMES64>(nframes);
}

} // namespace ici

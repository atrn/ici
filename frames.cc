#define ICI_CORE
#include "frames.h"
#include "int.h"
#include "float.h"
#include "forall.h"
#include "map.h"
#include "null.h"
#include "str.h"
#include "archiver.h"

namespace ici
{

template struct frames<TC_FRAMES32, float>;
template struct frames<TC_FRAMES64, double>;

namespace
{

template <typename frames> frames *new_frames(size_t nframes, size_t count, object *props)
{
    auto f = ici_talloc<frames>();
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
    f->_ptr = static_cast<typename frames::value_type *>(ici_alloc(nframes * sizeof (typename frames::value_type)));
    if (!f->_ptr)
    {
        ici_free(f);
        return nullptr;
    }
    f->set_tfnz(frames::type_code, 0, 1, 0);
    f->_size = nframes;
    f->_count = count;
    rego(f);
    return f;
}

template <typename frames> object *dofetch(frames *f, object *k)
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

template <typename frames> int doassign(frames *f, object *k, object *v)
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
            (*f)[ofs] = static_cast<typename frames::value_type>(intof(v)->i_value);
            ++f->_count;
            return 0;
        }
        if (isfloat(v))
        {
            (*f)[ofs] = static_cast<typename frames::value_type>(floatof(v)->f_value);
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

template <typename frames> int doforall(object *o)
{
    auto     fa = forallof(o);

    auto f = static_cast<frames *>(fa->fa_aggr);
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

template <typename frames> int dosave(archiver *ar, frames *f)
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

template <typename frames> object *dorestore(archiver *ar)
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

    auto f = make_ref<frames>(new_frames<frames>(size, count, props));
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

size_t frames32_type::mark(object *o)
{
    return type::mark(o)
        + frames32of(o)->_props->mark()
        + frames32of(o)->_size * sizeof (frames32::value_type);
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

int frames32_type::save(archiver *ar, object *o)
{
    return dosave(ar, frames32of(o));
}

object * frames32_type::restore(archiver *ar)
{
    return dorestore<frames32>(ar);
}

frames32 *new_frames32(size_t nframes, size_t count, object *props)
{
    return new_frames<frames32>(nframes, count, props);
}

//  ----------------------------------------------------------------

size_t frames64_type::mark(object *o)
{
    return type::mark(o)
        + frames64of(o)->_props->mark()
        + frames64of(o)->_size * sizeof(frames64::value_type);
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

int frames64_type::save(archiver *ar, object *o)
{
    return dosave(ar, frames64of(o));
}

object * frames64_type::restore(archiver *ar)
{
    return dorestore<frames64>(ar);
}

frames64 *new_frames64(size_t nframes, size_t count, object *props)
{
    return new_frames<frames64>(nframes, count, props);
}

} // namespace ici

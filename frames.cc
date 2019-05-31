#define ICI_CORE
#include "frames.h"
#include "int.h"
#include "float.h"
#include "map.h"
#include "str.h"

namespace ici
{

namespace
{

template <typename T, const int TCODE, typename C>
T *new_frames(size_t nframes)
{
    auto f = ici_talloc<T>();
    if (f)
    {
        f->_props = new_map();
        if (!f->_props) {
            return nullptr;
        }
        f->_ptr = static_cast<C *>(ici_alloc(nframes * sizeof (C)));
        if (!f->_ptr)
        {
            ici_free(f);
            return nullptr;
        }
        f->set_tfnz(TCODE, 0, 1, 0);
        f->_size = nframes;
        f->_count = 0;
        rego(f);
    }
    return f;
}

template <typename FRAME>
object *dofetch(FRAME *f, object *k)
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

template <typename FRAME>
int doassign(FRAME *f, object *k, object *v)
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
            (*f)[ofs] = static_cast<float>(intof(v)->i_value);
            ++f->_count;
            return 0;
        }
        else if (isfloat(v))
        {
            (*f)[ofs] = static_cast<float>(floatof(v)->f_value);
            ++f->_count;
            return 0;
        }
    }

    if (k == SS(count))
    {
        if (isint(v))
        {
            auto count = intof(v)->i_value;
            if (count < 0 || static_cast<size_t>(count) > f->_size)
            {
                return set_error("count out of range");
            }
            f->_count = count;
            return 0;
        }
    }
    
    return f->_props->assign(k, v);
}

} // anon


//  ----------------------------------------------------------------

size_t frames32_type::mark(object *o)
{
    return type::mark(o)
        + frames32of(o)->_size * sizeof (float)
        + frames32of(o)->_props->mark();
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

frames32 *new_frames32(size_t nframes)
{
    return new_frames<frames32, TC_FRAMES32, float>(nframes);
}

//  ----------------------------------------------------------------

size_t frames64_type::mark(object *o)
{
    return type::mark(o)
        + frames64of(o)->_size * sizeof (double)
        + frames64of(o)->_props->mark();
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

frames64 *new_frames64(size_t nframes)
{
    return new_frames<frames64, TC_FRAMES64, double>(nframes);
}

} // namespace ici

#include <ici.h>
#include <cmath>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

static int f_channel()
{
    ici::object *vec;
    ici::object *out;
    int64_t     channel;
    int64_t     stride;

    if (typecheck("oii", &vec, &channel, &stride))
    {
        return 1;
    }
    if (channel <= 0)
    {
        return ici::argerror(1);
    }
    if (stride <= 0)
    {
        return ici::argerror(2);
    }
    if (ici::isvec32(vec))
    {
        const auto size = size_t(ceil(ici::vec32of(vec)->v_size / double(stride)));
        out = ici::new_vec32(size);
        for (size_t i = 0, j = 0; i < size; i += stride, ++j)
        {
            ici::vec32of(out)->v_ptr[j] = ici::vec32of(vec)->v_ptr[i+channel-1];
        }
        ici::vec32of(out)->v_size = size;
    }
    else if (ici::isvec64(vec))
    {
        const auto size = size_t(ceil(ici::vec64of(vec)->v_size / double(stride)));
        out = ici::new_vec64(size);
        for (size_t i = 0, j = 0; i < size; i += stride, ++j)
        {
            ici::vec64of(out)->v_ptr[j] = ici::vec64of(vec)->v_ptr[i+channel-1];
        }
        ici::vec64of(out)->v_size = size;
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::ret_with_decref(out);
}

static int f_fill()
{
    ici::object *vec;
    double value;
    if (ici::typecheck("on", &vec, &value))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        vec32of(vec)->fill(value);
    }
    else if(ici::isvec64(vec))
    {
        vec64of(vec)->fill(value);
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

static int f_randomize()
{
    auto noise = []() -> double
    {
        return rand() / double(RAND_MAX);
    };
    
    ici::object *vec;
    if (typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        for (size_t i = 0; i < ici::vec32of(vec)->v_capacity; ++i)
        {
            ici::vec32of(vec)->v_ptr[i] = float(noise());
        }
        ici::vec32of(vec)->v_size = ici::vec32of(vec)->v_capacity;
    }
    else if (isvec64(vec))
    {
        for (size_t i = 0; i < ici::vec64of(vec)->v_capacity; ++i)
        {
            ici::vec64of(vec)->v_ptr[i] = noise();
        }
        ici::vec64of(vec)->v_size = ici::vec64of(vec)->v_capacity;
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

template <typename FLOAT>
void normalize(FLOAT *data, size_t sz)
{
    FLOAT avg = 0;

    for (size_t i = 0; i < sz; ++i)
    {
        avg += data[i];
    }
    avg /= sz;
    for (size_t i = 0; i < sz; i++)
    {
        data[i] -= avg;
    }
    FLOAT sum = 0;
    for (size_t i = 0; i < sz; ++i)
    {
        const FLOAT x = data[i];
        sum += x * x;
    }
    const FLOAT dev = sqrt(sum / sz);
    if (dev != 0.0)
    {
        for (size_t i = 0; i < sz; ++i)
        {
            data[i] /= dev;
        }
    }
}

static int f_normalize()
{
    ici::object *vec;
    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        normalize<float>(vec32of(vec)->v_ptr, vec32of(vec)->v_size);
    }
    else if(ici::isvec64(vec))
    {
        normalize<double>(vec64of(vec)->v_ptr, vec64of(vec)->v_size);
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

} // anon

extern "C" ici::object *ici_vec_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "vec"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(vec)
    {
        ICI_DEFINE_CFUNC(channel, f_channel),
        ICI_DEFINE_CFUNC(fill, f_fill),
        ICI_DEFINE_CFUNC(normalize, f_normalize),
        ICI_DEFINE_CFUNC(randomize, f_randomize),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(vec));
}

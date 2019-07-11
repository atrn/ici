#include <ici.h>
#include <cmath>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

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

extern "C" ici::object *ici_vector_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "vector"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(vector)
    {
        ICI_DEFINE_CFUNC(fill, f_fill),
        ICI_DEFINE_CFUNC(normalize, f_normalize),
        ICI_DEFINE_CFUNC(randomize, f_randomize),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(vector));
}

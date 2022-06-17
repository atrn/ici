#include <ici.h>
#include <cmath>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

/**
 * vec = vec.channel(vec, index, stride)
 *
 * Return a new vec comprising the values of every index'th element
 * from the input vec that contains a total of <stide> channels.
 */
int f_channel()
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
    if (ici::isvec32f(vec))
    {
        const auto size = size_t(ceil(ici::vec32fof(vec)->v_size / double(stride)));
        out = ici::new_vec32f(size);
        for (size_t i = 0, j = 0; i < size; i += stride, ++j)
        {
            ici::vec32fof(out)->v_ptr[j] = ici::vec32fof(vec)->v_ptr[i+channel-1];
        }
        ici::vec32fof(out)->resize(size);
    }
    else if (ici::isvec64f(vec))
    {
        const auto size = size_t(ceil(ici::vec64fof(vec)->v_size / double(stride)));
        out = ici::new_vec64f(size);
        for (size_t i = 0, j = 0; i < size; i += stride, ++j)
        {
            ici::vec64fof(out)->v_ptr[j] = ici::vec64fof(vec)->v_ptr[i+channel-1];
        }
        ici::vec64fof(out)->resize(size);
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::ret_with_decref(out);
}

/*
 * Helper function for f_merge, see below.
 */
template <typename VEC>
void merge(VEC *result, size_t maxsize, ici::object **vec, size_t nvec)
{
    size_t j = 0;
    for (size_t i = 0; i < maxsize; ++i)
    {
        for (size_t k = 0; k < nvec; ++k)
        {
            auto v = static_cast<VEC *>(vec[-k]); // see docs for ici::NARGS()
            if (i < v->size())
            {
                (*result)[j] = (*v)[i];
            }
            else
            {
                (*result)[j] = typename VEC::value_type(0.0);
            }
            ++j;
        }
    }
}

/*
 * vec = merge(vec...)
 *
 * Merge the data from two or more input vecs to interleave
 * their values, i.e. a complement function to channel()
 */
int f_merge()
{
    if (ici::NARGS() == 1)
    {
        if (!ici::isvec32f(ici::ARG(0)) && !ici::isvec64f(ici::ARG(0)))
        {
            return ici::argerror(0);
        }
	return ici::ret_with_decref(ici::ARG(0));
    }

    const auto tcode = ici::ARG(0)->o_tcode;

    size_t size = ici::vec_size(ici::ARG(0));
    size_t maxsize = size;

    for (int i = 1; i < ici::NARGS(); ++i)
    {
        if (ici::ARG(i)->o_tcode != tcode)
        {
            if (!ici::isvec32f(ici::ARG(i)) && !ici::isvec64f(ici::ARG(i)))
            {
                return ici::argerror(i);
            }
            return ici::set_errorc("cannot merge vectors of different types");
        }
        const auto z = ici::vec_size(ici::ARG(i));
        if (z > maxsize)
        {
            maxsize = z;
        }
        const auto newsize = size + z;
        if (newsize < size)
        {
            return ici::set_errorc("merged vector is too large");
        }
        size = newsize;
    }

    ici::object *result;
    if (tcode == ici::TC_VEC32F)
    {
        result = ici::new_vec32f(size);
        if (!result)
        {
            return 1;
        }
        merge<ici::vec32f>(vec32fof(result), maxsize, ici::ARGS(), ici::NARGS());
    }
    else
    {
        result = ici::new_vec64f(size);
        if (!result)
        {
            return 1;
        }
        merge<ici::vec64f>(vec64fof(result), maxsize, ici::ARGS(), ici::NARGS());
    }

    return ici::ret_with_decref(result);
}

/*
 * Fill a vec with a constant value.
 */
int f_fill()
{
    ici::object *vec;
    double value;
    if (ici::typecheck("on", &vec, &value))
    {
        return 1;
    }
    if (ici::isvec32f(vec))
    {
        vec32fof(vec)->fill(value);
    }
    else if(ici::isvec64f(vec))
    {
        vec64fof(vec)->fill(value);
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

/*
 * Fill a vec with random values.
 */
int f_randomize()
{
    auto noisef = []()
    {
        return 2.0f * (float(rand()) / float(RAND_MAX)) - 1.0f;
    };

    auto noise = []()
    {
        return 2.0 * (double(rand()) / double(RAND_MAX)) - 1.0;
    };

    ici::object *vec;
    if (typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32f(vec))
    {
        for (size_t i = 0; i < ici::vec32fof(vec)->v_capacity; ++i)
        {
            ici::vec32fof(vec)->v_ptr[i] = noisef();
        }
        ici::vec32fof(vec)->resize();
    }
    else if (isvec64f(vec))
    {
        for (size_t i = 0; i < ici::vec64fof(vec)->v_capacity; ++i)
        {
            ici::vec64fof(vec)->v_ptr[i] = noise();
        }
        ici::vec64fof(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

/*
 * Helper for f_normalize, see below.
 */
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

/*
 * Normalize the values held within a vec.
 */
int f_normalize()
{
    ici::object *vec;
    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32f(vec))
    {
        normalize<float>(vec32fof(vec)->v_ptr, vec32fof(vec)->v_size);
    }
    else if(ici::isvec64f(vec))
    {
        normalize<double>(vec64fof(vec)->v_ptr, vec64fof(vec)->v_size);
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
        ICI_DEFINE_CFUNC(merge, f_merge),
        ICI_DEFINE_CFUNC(normalize, f_normalize),
        ICI_DEFINE_CFUNC(randomize, f_randomize),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(vec));
}

#include <ici.h>

#include <ippcore.h>
#include <ipps.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

static int f_init()
{
    auto error = ippInit();
    if (error != ippStsNoErr)
    {
        return ici::set_error("ipp error %d", int(error));
    }
    return ici::null_ret();
}

#define DEFINE_INPLACE_NULLARY_OP(NAME, F32, F64)       \
    static int f_ ## NAME ()                            \
    {                                                   \
        ici::object *o;                                 \
        if (ici::typecheck("o", &o))                    \
        {                                               \
            return 1;                                   \
        }                                               \
        if (ici::isvec32(o))                            \
        {                                               \
            auto vec = ici::vec32of(o);                 \
            F32 ;                                       \
        }                                               \
        else if(ici::isvec64(o))                        \
        {                                               \
            auto vec = ici::vec64of(o);                 \
            F64 ;                                       \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        return ici::null_ret();                         \
    }

#define DEFINE_INPLACE_OP(NAME, F32, F64)               \
    static int f_ ## NAME ()                            \
    {                                                   \
        ici::object *o;                                 \
        double arg;                                     \
        if (ici::typecheck("on", &o, &arg))             \
        {                                               \
            return 1;                                   \
        }                                               \
        if (ici::isvec32(o))                            \
        {                                               \
            auto vec = ici::vec32of(o);                 \
            F32 ;                                       \
        }                                               \
        else if(ici::isvec64(o))                        \
        {                                               \
            auto vec = ici::vec64of(o);                 \
            F64 ;                                       \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        return ici::null_ret();                         \
    }

DEFINE_INPLACE_NULLARY_OP
(
    exp,
    ippsExp_32f_I(vec->v_ptr, int(vec->v_size)),
    ippsExp_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    ln,
    ippsLn_32f_I(vec->v_ptr, int(vec->v_size)),
    ippsLn_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_OP
(
    set,
    ippsSet_32f(arg, vec->v_ptr, int(vec->v_size)),
    ippsSet_64f(arg, vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    sqrt,
    ippsSqrt_32f_I(vec->v_ptr, int(vec->v_size)),
    ippsSqrt_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    zero,
    ippsZero_32f(vec->v_ptr, int(vec->v_size)),
    ippsZero_64f(vec->v_ptr, int(vec->v_size))
)

static int f_tone()
{
    ici::object *vec;
    double magnitude, frequency, phase;
    int64_t hint = 0;

    switch (ici::NARGS())
    {
    case 5:
        if (ici::typecheck("onnnd", &vec, &magnitude, &frequency, &phase, &hint))
        {
            return 1;
        }
        break;
    case 4:
        if (ici::typecheck("onnn", &vec, &magnitude, &frequency, &phase))
        {
            return 1;
        }
        break;
    case 3:
        if (ici::typecheck("onn", &vec, &magnitude, &frequency))
        {
            return 1;
        }
        break;
    default:
        return ici::argcount(3);
    }
    if (ici::isvec32(vec))
    {
        float phase32 = float(phase);
        ippsTone_32f
        (
            ici::vec32of(vec)->v_ptr, int(ici::vec32of(vec)->v_capacity),
            float(magnitude), float(frequency), &phase32,
            IppHintAlgorithm(hint)
        );
        ici::vec32of(vec)->resize();
    }
    else if (ici::isvec64(vec))
    {
        ippsTone_64f
        (
            ici::vec64of(vec)->v_ptr, int(ici::vec64of(vec)->v_capacity),
            float(magnitude), float(frequency), &phase,
            IppHintAlgorithm(hint)
        );
        ici::vec64of(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::null_ret();
}

} // anon

extern "C" ici::object *ici_ipp_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "ipp"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(ipp)
    {
        ICI_DEFINE_CFUNC(exp,  f_exp),
        ICI_DEFINE_CFUNC(init, f_init),
        ICI_DEFINE_CFUNC(ln,   f_ln),
        ICI_DEFINE_CFUNC(set,  f_set),
        ICI_DEFINE_CFUNC(sqrt, f_sqrt),
        ICI_DEFINE_CFUNC(tone, f_tone),
        ICI_DEFINE_CFUNC(zero, f_zero),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(ipp));
}

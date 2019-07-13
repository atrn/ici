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

/*
 * float = ipp.tone(vec, mag, freq [, phase [, alg]])
 *
 * Fills vec to capacity with a (co)sine wave of the
 * given magnitude and frequency starting with the
 * supplied initial phase, or 0.0. The optional
 * 'alg' argument specifies the algorithm to use.
 *
 * Returns the next initial phase value.
 */
static int f_tone()
{
    ici::object *       vec;
    double              magnitude;
    double              frequency;
    double              phase;
    int64_t             hint = ippAlgHintAccurate;

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
        phase = 0.0;
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
            float(magnitude),
            float(frequency),
            &phase32,
            IppHintAlgorithm(hint)
        );
        ici::vec32of(vec)->resize();
        phase = phase32;
    }
    else if (ici::isvec64(vec))
    {
        ippsTone_64f
        (
            ici::vec64of(vec)->v_ptr, int(ici::vec64of(vec)->v_capacity),
            magnitude,
            frequency,
            &phase,
            IppHintAlgorithm(hint)
        );
        ici::vec64of(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    return ici::float_ret(phase);
}

} // anon

// ----------------------------------------------------------------

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
    auto module = ici::new_module(ICI_CFUNCS(ipp));

#define DEFINE_CONST(NAME)                                              \
    do                                                                  \
    {                                                                   \
        auto key = ici::make_ref(ici::new_str_nul_term(#NAME));         \
        auto val = ici::make_ref(ici::new_int(ipp ## NAME));            \
        module->assign(key, val);                                       \
    }                                                                   \
    while (0)

    DEFINE_CONST(RndZero);
    DEFINE_CONST(RndNear);
    DEFINE_CONST(RndFinancial);
    DEFINE_CONST(RndHintAccurate);

    DEFINE_CONST(AlgHintNone);
    DEFINE_CONST(AlgHintFast);
    DEFINE_CONST(AlgHintAccurate);

    DEFINE_CONST(CmpLess);
    DEFINE_CONST(CmpLessEq);
    DEFINE_CONST(CmpEq);
    DEFINE_CONST(CmpGreaterEq);
    DEFINE_CONST(CmpGreater);

    DEFINE_CONST(AlgAuto);
    DEFINE_CONST(AlgDirect);
    DEFINE_CONST(AlgFFT);
    DEFINE_CONST(AlgMask);

    DEFINE_CONST(NormInf);
    DEFINE_CONST(NormL1);
    DEFINE_CONST(NormL2);

    return module;
}

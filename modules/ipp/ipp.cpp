#include <ici.h>

#include <ippcore.h>
#include <ipps.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

int check_error(int code)
{
    if (code != ippStsNoErr)
    {
        return ici::set_error("%s", ippGetStatusString(code));
    }
    return 0;
}

int f_init()
{
    const int error = ippInit();
    if (check_error(error))
    {
        return 1;
    }
    return ici::null_ret();
}

#define DEFINE_INPLACE_NULLARY_OP(NAME, CODE32, CODE64) \
    int f_ ## NAME ()                                   \
    {                                                   \
        ici::object *o;                                 \
        int error = ippStsNoErr;                        \
        if (ici::typecheck("o", &o))                    \
        {                                               \
            return 1;                                   \
        }                                               \
        if (ici::isvec32(o))                            \
        {                                               \
            auto vec = ici::vec32of(o);                 \
            CODE32 ;                                    \
        }                                               \
        else if(ici::isvec64(o))                        \
        {                                               \
            auto vec = ici::vec64of(o);                 \
            CODE64 ;                                    \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        if (check_error(error))                         \
        {                                               \
            return 1;                                   \
        }                                               \
        return ici::null_ret();                         \
    }

#define DEFINE_INPLACE_OP(NAME, CODE32, CODE64)         \
    int f_ ## NAME ()                                   \
    {                                                   \
        ici::object *o;                                 \
        double arg;                                     \
        int error = ippStsNoErr;                        \
        if (ici::typecheck("on", &o, &arg))             \
        {                                               \
            return 1;                                   \
        }                                               \
        if (ici::isvec32(o))                            \
        {                                               \
            auto vec = ici::vec32of(o);                 \
            CODE32 ;                                    \
        }                                               \
        else if(ici::isvec64(o))                        \
        {                                               \
            auto vec = ici::vec64of(o);                 \
            CODE64 ;                                    \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        if (check_error(error))                         \
        {                                               \
            return 1;                                   \
        }                                               \
        return ici::null_ret();                         \
    }

DEFINE_INPLACE_NULLARY_OP
(
    abs,
    error = ippsAbs_32f_I(vec->v_ptr, int(vec->v_size)),
    error = ippsAbs_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    exp,
    error = ippsExp_32f_I(vec->v_ptr, int(vec->v_size)),
    error = ippsExp_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    ln,
    error = ippsLn_32f_I(vec->v_ptr, int(vec->v_size)),
    error = ippsLn_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_OP
(
    set,
    error = ippsSet_32f(arg, vec->v_ptr, int(vec->v_size)),
    error = ippsSet_64f(arg, vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    sqr,
    error = ippsSqr_32f_I(vec->v_ptr, int(vec->v_size)),
    error = ippsSqr_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    sqrt,
    error = ippsSqrt_32f_I(vec->v_ptr, int(vec->v_size)),
    error = ippsSqrt_64f_I(vec->v_ptr, int(vec->v_size))
)

DEFINE_INPLACE_NULLARY_OP
(
    zero,
    error = ippsZero_32f(vec->v_ptr, int(vec->v_size)),
    error = ippsZero_64f(vec->v_ptr, int(vec->v_size))
)

/*
 * float = ipp.tone(vec, mag, rfreq [, phase [, alg]])
 *
 * Fills vec to capacity with a (co)sine wave of the
 * given magnitude and frequency starting with the
 * supplied initial phase, or 0.0. The optional
 * 'alg' argument specifies the algorithm to use.
 *
 * Returns the next initial phase value.
 */
int f_tone()
{
    ici::object *       vec;
    double              magnitude;
    double              rfrequency;
    double              phase;
    int64_t             hint = ippAlgHintAccurate;

    switch (ici::NARGS())
    {
    case 5:
        if (ici::typecheck("onnnd", &vec, &magnitude, &rfrequency, &phase, &hint))
        {
            return 1;
        }
        break;
    case 4:
        if (ici::typecheck("onnn", &vec, &magnitude, &rfrequency, &phase))
        {
            return 1;
        }
        break;
    case 3:
        if (ici::typecheck("onn", &vec, &magnitude, &rfrequency))
        {
            return 1;
        }
        phase = 0.0;
        break;
    default:
        return ici::argcount(3);
    }
    int error = ippStsNoErr;
    if (ici::isvec32(vec))
    {
        float phase32 = float(phase);
        error = ippsTone_32f
        (
            ici::vec32of(vec)->v_ptr, int(ici::vec32of(vec)->v_capacity),
            float(magnitude),
            float(rfrequency),
            &phase32,
            IppHintAlgorithm(hint)
        );
        ici::vec32of(vec)->resize();
        phase = phase32;
    }
    else if (ici::isvec64(vec))
    {
        error = ippsTone_64f
        (
            ici::vec64of(vec)->v_ptr, int(ici::vec64of(vec)->v_capacity),
            magnitude,
            rfrequency,
            &phase,
            IppHintAlgorithm(hint)
        );
        ici::vec64of(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::float_ret(phase);
}

/*
 * float = ipp.triangle(vec, mag, freq [, phase [, asym]])
 *
 * Fills vec to capacity with a triangular wave of the given magnitude
 * and frequency starting with the supplied initial phase, or 0.0. The
 * optional 'alg' argument specifies the algorithm to use.
 *
 * Returns the next initial phase value.
 */
int f_triangle()
{
    ici::object *       vec;
    double              magnitude;
    double              rfrequency;
    double              phase;
    double              asym;
    int                 error = ippStsNoErr;

    switch (ici::NARGS())
    {
    case 5:
        if (ici::typecheck("onnnn", &vec, &magnitude, &rfrequency, &phase, &asym))
        {
            return 1;
        }
        break;
    case 4:
        if (ici::typecheck("onnn", &vec, &magnitude, &rfrequency, &phase))
        {
            return 1;
        }
        asym = 0.0;
        break;
    case 3:
        if (ici::typecheck("onn", &vec, &magnitude, &rfrequency))
        {
            return 1;
        }
        asym = 0.0;
        phase = 0.0;
        break;
    default:
        return ici::argcount(3);
    }
    if (ici::isvec32(vec))
    {
        float phase32 = float(phase);
        error = ippsTriangle_32f
        (
            ici::vec32of(vec)->v_ptr, int(ici::vec32of(vec)->v_capacity),
            float(magnitude),
            float(rfrequency),
            float(asym),
            &phase32
        );
        ici::vec32of(vec)->resize();
        phase = phase32;
    }
    else if (ici::isvec64(vec))
    {
        error = ippsTriangle_64f
        (
            ici::vec64of(vec)->v_ptr, int(ici::vec64of(vec)->v_capacity),
            magnitude,
            rfrequency,
            asym,
            &phase
        );
        ici::vec64of(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::float_ret(phase);
}

int f_vector_slope()
{
    ici::object *vec;
    double offset;
    double slope;
    int error;

    if (ici::typecheck("onn", &vec, &offset, &slope))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        error = ippsVectorSlope_32f(vec32of(vec)->v_ptr, int(vec32of(vec)->v_capacity), float(offset), float(slope));
    }
    else if (ici::isvec64(vec))
    {
        error = ippsVectorSlope_64f(vec64of(vec)->v_ptr, int(vec64of(vec)->v_capacity), offset, slope);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::null_ret();
}

int f_max()
{
    ici::object *vec;
    double maxval;
    int error;

    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        float max32;
        error = ippsMax_32f(vec32of(vec)->v_ptr, int(vec32of(vec)->v_size), &max32);
        maxval = max32;
    }
    else if (ici::isvec64(vec))
    {
        error = ippsMax_64f(vec64of(vec)->v_ptr, int(vec64of(vec)->v_size), &maxval);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::float_ret(maxval);
}

int f_min()
{
    ici::object *vec;
    double minval;
    int error;

    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        float min32;
        error = ippsMin_32f(vec32of(vec)->v_ptr, int(vec32of(vec)->v_size), &min32);
        minval = min32;
    }
    else if (ici::isvec64(vec))
    {
        error = ippsMin_64f(vec64of(vec)->v_ptr, int(vec64of(vec)->v_size), &minval);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::float_ret(minval);
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
        ICI_DEFINE_CFUNC(abs,           f_abs),
        ICI_DEFINE_CFUNC(exp,           f_exp),
        ICI_DEFINE_CFUNC(init,          f_init),
        ICI_DEFINE_CFUNC(ln,            f_ln),
        ICI_DEFINE_CFUNC(min,           f_min),
        ICI_DEFINE_CFUNC(max,           f_max),
        ICI_DEFINE_CFUNC(set,           f_set),
        ICI_DEFINE_CFUNC(sqr,           f_sqr),
        ICI_DEFINE_CFUNC(sqrt,          f_sqrt),
        ICI_DEFINE_CFUNC(tone,          f_tone),
        ICI_DEFINE_CFUNC(triangle,      f_triangle),
        ICI_DEFINE_CFUNC(vector_slope,  f_vector_slope),
        ICI_DEFINE_CFUNC(zero,          f_zero),
        ICI_CFUNCS_END()
    };
    auto module = ici::new_module(ICI_CFUNCS(ipp));
    if (!module)
    {
        return nullptr;
    }

#define DEFINE_INT_VALUE(NAME)                                          \
    do                                                                  \
    {                                                                   \
        auto key = ici::make_ref(ici::new_str_nul_term(#NAME));         \
        auto val = ici::make_ref(ici::new_int(ipp ## NAME));            \
        if (module->assign(key, val))                                   \
        {                                                               \
            return nullptr;                                             \
        }                                                               \
    }                                                                   \
    while (0)

    DEFINE_INT_VALUE(RndZero);
    DEFINE_INT_VALUE(RndNear);
    DEFINE_INT_VALUE(RndFinancial);
    DEFINE_INT_VALUE(RndHintAccurate);

    DEFINE_INT_VALUE(AlgHintNone);
    DEFINE_INT_VALUE(AlgHintFast);
    DEFINE_INT_VALUE(AlgHintAccurate);

    DEFINE_INT_VALUE(CmpLess);
    DEFINE_INT_VALUE(CmpLessEq);
    DEFINE_INT_VALUE(CmpEq);
    DEFINE_INT_VALUE(CmpGreaterEq);
    DEFINE_INT_VALUE(CmpGreater);

    DEFINE_INT_VALUE(AlgAuto);
    DEFINE_INT_VALUE(AlgDirect);
    DEFINE_INT_VALUE(AlgFFT);
    DEFINE_INT_VALUE(AlgMask);

    DEFINE_INT_VALUE(NormInf);
    DEFINE_INT_VALUE(NormL1);
    DEFINE_INT_VALUE(NormL2);

    return module;
}

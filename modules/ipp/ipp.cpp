/*
 * ICI IPP interface.
 *
 * Copyright (C) A.Newman 2019.
 */

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

int f_minmax()
{
    ici::object *vec;
    double minval;
    double maxval;
    int error;

    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        float min32;
        float max32;
        error = ippsMinMax_32f(vec32of(vec)->v_ptr, int(vec32of(vec)->v_size), &min32, &max32);
        minval = min32;
        maxval = max32;
    }
    else if (ici::isvec64(vec))
    {
        error = ippsMinMax_64f(vec64of(vec)->v_ptr, int(vec64of(vec)->v_size), &minval, &maxval);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    auto r = ici::make_ref(ici::new_map());
    if (!r)
    {
        return 1;
    }
    auto v = ici::make_ref(ici::new_float(minval));
    if (!v)
    {
        return 1;
    }
    if (r->assign(ICIS(min), v))
    {
        return 1;
    }
    v = ici::make_ref(ici::new_float(maxval));
    if (!v)
    {
        return 1;
    }
    if (r->assign(ICIS(max), v))
    {
        return 1;
    }
    return ici::ret_with_decref(r);
}

// vec = ipp.normalize(vec, sub, div)
//      Inplace normalization of vector data via ippsNormalize.
//
// Returns its 1st argument to permit function call chaining.
//
int f_normalize()
{
    ici::object *vec;
    double sub;
    double div;
    int error;

    if (ici::typecheck("onn", &vec, &sub, &div))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        error = ippsNormalize_32f_I(ici::vec32of(vec)->v_ptr, ici::vec32of(vec)->v_size, float(sub), float(div));
    }
    else if (ici::isvec64(vec))
    {
        error = ippsNormalize_64f_I(ici::vec64of(vec)->v_ptr, ici::vec64of(vec)->v_size, sub, div);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::ret_no_decref(vec);
}

// vec = ipp.normalized(vec, sub, div)
//      Non-inplace normalization of vector data via ippsNormalize.
//
// Returns a new vec containing the result of normalizing the
// input vec with the given sub and div parameters.
//
int f_normalized()
{
    ici::object *result;
    ici::object *vec;
    double sub;
    double div;
    int error;

    if (ici::typecheck("onn", &vec, &sub, &div))
    {
        return 1;
    }
    if (ici::isvec32(vec))
    {
        auto v = ici::vec32of(vec);
        if (!(result = ici::new_vec32(v->v_size, v->v_size)))
        {
            return 1;
        }
        error = ippsNormalize_32f(v->v_ptr, ici::vec32of(result)->v_ptr, v->v_size, float(sub), float(div));
    }
    else if (ici::isvec64(vec))
    {
        auto v = ici::vec64of(vec);
        if (!(result = ici::new_vec64(v->v_size, v->v_size)))
        {
            return 1;
        }
        error = ippsNormalize_64f(v->v_ptr, ici::vec64of(result)->v_ptr, v->v_size, sub, div);
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::ret_with_decref(result);
}

// vec = ipp.add(vec, vec)
//      Non-inplace addition of equal size vectors. Returns a new vector.
//
// vec = ipp.add(vec, int|float)
//      Inplace addition of constant to vector. Returns its input vector.
//
int f_add()
{
    ici::object *vec;
    ici::object *rhs;
    double constant;
    int error;

    auto size_mismatch = [](size_t z1, size_t z2) -> int
    {
        return z1 == z2 ? 0 : ici::set_error("vector size mis-match, %lu vs %lu", z1, z2);
    };

    if (ici::typecheck("on", &vec, &constant) == 0)
    {
        if (ici::isvec32(vec))
        {
            error = ippsAddC_32f_I(float(constant), vec32of(vec)->v_ptr, int(vec32of(vec)->v_size));
        }
        else if (ici::isvec64(vec))
        {
            error = ippsAddC_64f_I(constant, vec64of(vec)->v_ptr, int(vec64of(vec)->v_size));
        }
        else
        {
            return ici::argerror(0);
        }
        if (check_error(error))
        {
            return 1;
        }
        return ici::ret_no_decref(vec);
    }

    if (ici::typecheck("oo", &vec, &rhs))
    {
        return 1;
    }

    ici::object *result;

    if (ici::isvec32(vec))
    {
        if (ici::isvec32(rhs))
        {
            if (size_mismatch(vec32of(vec)->v_size, vec32of(rhs)->v_size))
            {
                return 1;
            }
            auto r = ici::make_ref(ici::new_vec32(ici::vec32of(vec)->v_size, ici::vec32of(vec)->v_size));
            if (!r)
            {
                return 1;
            }
            error = ippsAdd_32f(ici::vec32of(vec)->v_ptr, ici::vec32of(rhs)->v_ptr, r->v_ptr, int(r->v_size));
            result = r;
        }
        else
        {
            return ici::argerror(1);
        }
    }
    else if (ici::isvec64(vec))
    {
        if (ici::isvec64(rhs))
        {
            if (size_mismatch(vec64of(vec)->v_size, vec64of(rhs)->v_size))
            {
                return 1;
            }
            auto r = ici::make_ref(ici::new_vec64(ici::vec64of(vec)->v_size, ici::vec64of(vec)->v_size));
            if (!r)
            {
                return 1;
            }
            error = ippsAdd_64f(ici::vec64of(vec)->v_ptr, ici::vec64of(rhs)->v_ptr, r->v_ptr, int(r->v_size));
            result = r;
        }
        else
        {
            return ici::argerror(1);
        }
    }
    else
    {
        return ici::argerror(0);
    }
    if (check_error(error))
    {
        return 1;
    }
    return ici::ret_with_decref(result);
}

// ----------------------------------------------------------------
//
// FFT

int fft_tcode;

struct fft : ici::object
{
    int _specsize;
    int _specbufsize;
    int _workbufsize;

    Ipp8u *_spec;
    Ipp8u *_specbuf;
    Ipp8u *_workbuf;

    IppsFFTSpec_R_32f *_pspec;

    void init(int order, int flag, IppHintAlgorithm hint)
    {
        set_tfnz(fft_tcode, 0, 1, 0);
        ippsFFTGetSize_R_32f(order, flag, hint, &_specsize, &_specbufsize, &_workbufsize);
        _spec = ippsMalloc_8u(_specsize);
        _specbuf = ippsMalloc_8u(_specbufsize);
        _workbuf = ippsMalloc_8u(_workbufsize);
        ippsFFTInit_R_32f(&_pspec, order, flag, hint, _spec, _specbuf);
    }

    void fwd(ici::vec32 *src, ici::vec32 *dst)
    {
        dst->resize(src->v_size);
        ippsFFTFwd_RToPerm_32f(src->v_ptr, dst->v_ptr, _pspec, _workbuf);
    }

};

inline bool isfft(ici::object *o) { return o->hastype(fft_tcode); }
inline fft * fftof(ici::object *o) { return static_cast<fft *>(o); }

struct fft_type : ici::type
{
    fft_type() : ici::type("fft", sizeof (fft))
    {
    }

    size_t mark(ici::object *o) override
    {
        return type::mark(o) + fftof(o)->_specsize + fftof(o)->_specbufsize + fftof(o)->_workbufsize;
    }

    void free(ici::object *o) override
    {
        ippsFree(fftof(o)->_spec);
        ippsFree(fftof(o)->_specbuf);
        ippsFree(fftof(o)->_workbuf);
        type::free(o);
    }
}
fft_type;

fft *new_fft(int order, int flag = 0, IppHintAlgorithm hint = ippAlgHintAccurate)
{
    fft *f = ici::ici_talloc<fft>();
    if (f)
    {
        f->init(order, flag, hint);
    }
    return f;
}

//
// fft = ipp.fft(order, flag, hint)
//
//
int f_fft()
{
    int64_t order;
    int64_t flag;
    int64_t hint;

    if (ici::NARGS() == 1)
    {
        if (ici::typecheck("i", &order))
        {
            return 1;
        }
        flag = IPP_FFT_DIV_FWD_BY_N;
        hint = int64_t(ippAlgHintAccurate);
    }
    else if (ici::NARGS() == 2)
    {
        if (ici::typecheck("ii", &order, &flag))
        {
            return 1;
        }
        hint = int64_t(ippAlgHintAccurate);
    }
    else if (ici::typecheck("iii", &order, &flag, &hint))
    {
        return 1;
    }

    auto f = new_fft(int(order), int(flag), IppHintAlgorithm(hint));
    return ici::ret_with_decref(f);
}

// ipp.fwd(fft, src, dst)
//
int f_fft_fwd()
{
    fft *f;
    ici::vec32 *src;
    ici::vec32 *dst;
    if (ici::typecheck("ooo", &f, &src, &dst))
    {
        return 1;
    }
    if (!isfft(f))
    {
        return ici::argerror(0);
    }
    if (!ici::isvec32(src))
    {
        return ici::argerror(1);
    }
    if (!ici::isvec32(dst))
    {
        return ici::argerror(2);
    }
    f->fwd(src, dst);
    return ici::null_ret();
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
    fft_tcode = ici::register_type(&fft_type);
    if (fft_tcode == 0)
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(ipp)
    {
        ICI_DEFINE_CFUNC(abs,           f_abs),
        ICI_DEFINE_CFUNC(add,           f_add),
        ICI_DEFINE_CFUNC(exp,           f_exp),
        ICI_DEFINE_CFUNC(fft,           f_fft),
        ICI_DEFINE_CFUNC(fwd,           f_fft_fwd),
        ICI_DEFINE_CFUNC(init,          f_init),
        ICI_DEFINE_CFUNC(ln,            f_ln),
        ICI_DEFINE_CFUNC(min,           f_min),
        ICI_DEFINE_CFUNC(max,           f_max),
        ICI_DEFINE_CFUNC(minmax,        f_minmax),
        ICI_DEFINE_CFUNC(normalize,     f_normalize),
        ICI_DEFINE_CFUNC(normalized,    f_normalized),
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

#define DEFINE_INT_VALUE(PREFIX, NAME)                                  \
    do                                                                  \
    {                                                                   \
        auto key = ici::make_ref(ici::new_str_nul_term(#NAME));         \
        auto val = ici::make_ref(ici::new_int(PREFIX ## NAME));         \
        if (module->assign(key, val))                                   \
        {                                                               \
            return nullptr;                                             \
        }                                                               \
    }                                                                   \
    while (0)

    DEFINE_INT_VALUE(ipp, RndZero);
    DEFINE_INT_VALUE(ipp, RndNear);
    DEFINE_INT_VALUE(ipp, RndFinancial);
    DEFINE_INT_VALUE(ipp, RndHintAccurate);

    DEFINE_INT_VALUE(ipp, AlgHintNone);
    DEFINE_INT_VALUE(ipp, AlgHintFast);
    DEFINE_INT_VALUE(ipp, AlgHintAccurate);

    DEFINE_INT_VALUE(ipp, CmpLess);
    DEFINE_INT_VALUE(ipp, CmpLessEq);
    DEFINE_INT_VALUE(ipp, CmpEq);
    DEFINE_INT_VALUE(ipp, CmpGreaterEq);
    DEFINE_INT_VALUE(ipp, CmpGreater);

    DEFINE_INT_VALUE(ipp, AlgAuto);
    DEFINE_INT_VALUE(ipp, AlgDirect);
    DEFINE_INT_VALUE(ipp, AlgFFT);
    DEFINE_INT_VALUE(ipp, AlgMask);

    DEFINE_INT_VALUE(ipp, NormInf);
    DEFINE_INT_VALUE(ipp, NormL1);
    DEFINE_INT_VALUE(ipp, NormL2);

    DEFINE_INT_VALUE(IPP_, FFT_DIV_FWD_BY_N);
    DEFINE_INT_VALUE(IPP_, FFT_DIV_INV_BY_N);
    DEFINE_INT_VALUE(IPP_, FFT_DIV_BY_SQRTN);
    DEFINE_INT_VALUE(IPP_, FFT_NODIV_BY_ANY);

    return module;
}

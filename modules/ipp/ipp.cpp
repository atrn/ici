/*
 * ICI IPP interface.
 *
 * Copyright (C) A.Newman 2019.
 *
 * This is an ICI module providing access to functions defined by the
 * Intel IPP libraries.
 *
 * It is assumed, although not required, the module is used with a
 * IPP-enabled ici. This is not because of some code dependency or
 * anything like that but is because this module assumes various
 * IPP-functions are provided to the user by ici itself and are not
 * implemented in the module. Specifically, the ici _vec_ types are
 * assumed to provide basic vector/vector, vector/scalar arithmetic
 * operations that use IPP.
 *
 * Mapping the IPP C/C++ functions to ICI is straight forward and
 * where possible IPP-naming and argument orders are preserved
 * (allowing the C/C++ IPP documentation to be used). The general rule
 * is that when an IPP function takes a pointer to a vector and a
 * vector size the ici version uses a single _vec_ value, `vec32f` for
 * single-precision or `vec64f` for double-precision.
 *
 * Some operations are represented differently to better integrate
 * inplace and non-inplace operations. E.g. the `normalize` function
 * performs in-place normalization of data while the `normalized`
 * function is the non-inplace operation, returning a new value with
 * the normalized data.
 *
 * Performance
 *
 * The module is a relatively thin wrapper atop the IPP functions but
 * of course adds overhead. That overhead, however, is minimal when
 * compared to other overheads in an ici program's execution.
 */

#include <ici.h>

#include <ippcore.h>
#include <ipps.h>
#include <ippvm.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

inline int check_error(int code)
{
    if (code != ippStsNoErr)
    {
        return ici::set_error("%s", ippGetStatusString(code));
    }
    return 0;
}

template <typename Fn>
inline int check_error(int code, const Fn &fn)
{
    if (check_error(code))
    {
        return 1;
    }
    return fn();
}

int f_init()
{
    const int error = ippInit();
    return check_error(error, ici::null_ret);
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
        if (ici::isvec32f(o))                            \
        {                                               \
            auto vec = ici::vec32fof(o);                 \
            CODE32 ;                                    \
        }                                               \
        else if(ici::isvec64f(o))                        \
        {                                               \
            auto vec = ici::vec64fof(o);                 \
            CODE64 ;                                    \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        return check_error(error, ici::null_ret);       \
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
        if (ici::isvec32f(o))                            \
        {                                               \
            auto vec = ici::vec32fof(o);                 \
            CODE32 ;                                    \
        }                                               \
        else if(ici::isvec64f(o))                        \
        {                                               \
            auto vec = ici::vec64fof(o);                 \
            CODE64 ;                                    \
        }                                               \
        else                                            \
        {                                               \
            return ici::argerror(0);                    \
        }                                               \
        return check_error(error, ici::null_ret);       \
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

#define DEFINE_UNARY_OP(FUNC, OP32, OP64)                               \
    int FUNC ()                                                         \
    {                                                                   \
        ici::object *       vec;                                        \
        ici::object *       result;                                     \
                                                                        \
        int error = ippStsNoErr;                                        \
                                                                        \
        if (ici::typecheck("o", &vec))                                  \
        {                                                               \
            return 1;                                                   \
        }                                                               \
        if (ici::isvec32f(vec))                                         \
        {                                                               \
            auto v = ici::vec32fof(vec);                                \
            if (!(result = ici::new_vec32f(v->v_size, v->v_size)))      \
            {                                                           \
                return 1;                                               \
            }                                                           \
            error = OP32(v->v_ptr, ici::vec32fof(result)->v_ptr, v->v_size); \
        }                                                               \
        else if (ici::isvec64f(vec))                                    \
        {                                                               \
            auto v = ici::vec64fof(vec);                                \
            if (!(result = ici::new_vec64f(v->v_size, v->v_size)))      \
            {                                                           \
                return 1;                                               \
            }                                                           \
            error = OP64(v->v_ptr, ici::vec64fof(result)->v_ptr, v->v_size); \
        }                                                               \
        else                                                            \
        {                                                               \
            return ici::argerror(0);                                    \
        }                                                               \
        return check_error(error, [result]() { return ici::ret_with_decref(result); }); \
    }

/*
 * vec = ipp.cosh(vec)
 */
DEFINE_UNARY_OP(f_cosh, ippsCosh_32f_A21, ippsCosh_64f_A50)

/*
 * vec = ipp.sinh(vec)
 */
DEFINE_UNARY_OP(f_sinh, ippsSinh_32f_A21, ippsSinh_64f_A50)

/*
 * vec = ipp.tanh(vec)
 */
DEFINE_UNARY_OP(f_tanh, ippsTanh_32f_A21, ippsTanh_64f_A50)

/*
 * float = ipp.tone(vec, mag, rfreq [, phase [, alg]])
 *
 * Fills vec to capacity with a (co)sine wave of the given magnitude and
 * frequency starting with the supplied initial phase, or 0.0. The optional
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
    if (ici::isvec32f(vec))
    {
        float phase32 = float(phase);
        error = ippsTone_32f
        (
            ici::vec32fof(vec)->v_ptr, int(ici::vec32fof(vec)->v_capacity),
            float(magnitude),
            float(rfrequency),
            &phase32,
            IppHintAlgorithm(hint)
        );
        ici::vec32fof(vec)->resize();
        phase = phase32;
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsTone_64f
        (
            ici::vec64fof(vec)->v_ptr, int(ici::vec64fof(vec)->v_capacity),
            magnitude,
            rfrequency,
            &phase,
            IppHintAlgorithm(hint)
        );
        ici::vec64fof(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [phase]() { return ici::float_ret(phase); });
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
    if (ici::isvec32f(vec))
    {
        float phase32 = float(phase);
        error = ippsTriangle_32f
        (
            ici::vec32fof(vec)->v_ptr, int(ici::vec32fof(vec)->v_capacity),
            float(magnitude),
            float(rfrequency),
            float(asym),
            &phase32
        );
        ici::vec32fof(vec)->resize();
        phase = phase32;
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsTriangle_64f
        (
            ici::vec64fof(vec)->v_ptr, int(ici::vec64fof(vec)->v_capacity),
            magnitude,
            rfrequency,
            asym,
            &phase
        );
        ici::vec64fof(vec)->resize();
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [phase]() { return ici::float_ret(phase); });
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
    if (ici::isvec32f(vec))
    {
        error = ippsVectorSlope_32f(vec32fof(vec)->v_ptr, int(vec32fof(vec)->v_capacity), float(offset), float(slope));
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsVectorSlope_64f(vec64fof(vec)->v_ptr, int(vec64fof(vec)->v_capacity), offset, slope);
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, ici::null_ret);
}

/**
 * float = ipp.min(vec)
 */
int f_min()
{
    ici::object *vec;
    double minval;
    int error;

    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32f(vec))
    {
        float min32;
        error = ippsMin_32f(vec32fof(vec)->v_ptr, int(vec32fof(vec)->v_size), &min32);
        minval = min32;
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsMin_64f(vec64fof(vec)->v_ptr, int(vec64fof(vec)->v_size), &minval);
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [minval]() { return ici::float_ret(minval); });
}

/**
 * float = ipp.max(vec)
 */
int f_max()
{
    ici::object *vec;
    double maxval;
    int error;

    if (ici::typecheck("o", &vec))
    {
        return 1;
    }
    if (ici::isvec32f(vec))
    {
        float max32;
        error = ippsMax_32f(vec32fof(vec)->v_ptr, int(vec32fof(vec)->v_size), &max32);
        maxval = max32;
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsMax_64f(vec64fof(vec)->v_ptr, int(vec64fof(vec)->v_size), &maxval);
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [maxval]() { return ici::float_ret(maxval); });
}

/**
 * map = ipp.minmax(vec)
 *
 * map.min
 * map.max
 */
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
    if (ici::isvec32f(vec))
    {
        float min32;
        float max32;
        error = ippsMinMax_32f(vec32fof(vec)->v_ptr, int(vec32fof(vec)->v_size), &min32, &max32);
        minval = min32;
        maxval = max32;
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsMinMax_64f(vec64fof(vec)->v_ptr, int(vec64fof(vec)->v_size), &minval, &maxval);
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

/**
 * vec = ipp.normalize(vec, sub, div)
 *
 * Inplace normalization of vector data via ippsNormalize.
 *
 * Returns its 1st argument to permit function call chaining.
*/
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
    if (ici::isvec32f(vec))
    {
        error = ippsNormalize_32f_I(ici::vec32fof(vec)->v_ptr, ici::vec32fof(vec)->v_size, float(sub), float(div));
    }
    else if (ici::isvec64f(vec))
    {
        error = ippsNormalize_64f_I(ici::vec64fof(vec)->v_ptr, ici::vec64fof(vec)->v_size, sub, div);
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [vec]() { return ici::ret_no_decref(vec); });
}

/**
 * vec = ipp.normalized(vec, sub, div)
 *
 * Non-inplace normalization of vector data via ippsNormalize.
 *
 * Returns a new vec containing the result of normalizing the
 * input vec with the given sub and div parameters.
 */
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
    if (ici::isvec32f(vec))
    {
        auto v = ici::vec32fof(vec);
        if (!(result = ici::new_vec32f(v->v_size, v->v_size)))
        {
            return 1;
        }
        error = ippsNormalize_32f(v->v_ptr, ici::vec32fof(result)->v_ptr, v->v_size, float(sub), float(div));
    }
    else if (ici::isvec64f(vec))
    {
        auto v = ici::vec64fof(vec);
        if (!(result = ici::new_vec64f(v->v_size, v->v_size)))
        {
            return 1;
        }
        error = ippsNormalize_64f(v->v_ptr, ici::vec64fof(result)->v_ptr, v->v_size, sub, div);
    }
    else
    {
        return ici::argerror(0);
    }
    return check_error(error, [result]() { return ici::ret_with_decref(result); });
}

/**
 * vec = ipp.add(vec, vec)
 * vec = ipp.add(vec, int|float)
 *
 * Non-inplace addition of equal size vectors. Returns a new vector.
 *
 * Inplace addition of constant to vector. Returns the input vector.
 */
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
        if (ici::isvec32f(vec))
        {
            error = ippsAddC_32f_I(float(constant), vec32fof(vec)->v_ptr, int(vec32fof(vec)->v_size));
        }
        else if (ici::isvec64f(vec))
        {
            error = ippsAddC_64f_I(constant, vec64fof(vec)->v_ptr, int(vec64fof(vec)->v_size));
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

    if (ici::isvec32f(vec))
    {
        if (ici::isvec32f(rhs))
        {
            if (size_mismatch(vec32fof(vec)->v_size, vec32fof(rhs)->v_size))
            {
                return 1;
            }
            auto r = ici::make_ref(ici::new_vec32f(ici::vec32fof(vec)->v_size, ici::vec32fof(vec)->v_size));
            if (!r)
            {
                return 1;
            }
            error = ippsAdd_32f(ici::vec32fof(vec)->v_ptr, ici::vec32fof(rhs)->v_ptr, r->v_ptr, int(r->v_size));
            result = r;
        }
        else
        {
            return ici::argerror(1);
        }
    }
    else if (ici::isvec64f(vec))
    {
        if (ici::isvec64f(rhs))
        {
            if (size_mismatch(vec64fof(vec)->v_size, vec64fof(rhs)->v_size))
            {
                return 1;
            }
            auto r = ici::make_ref(ici::new_vec64f(ici::vec64fof(vec)->v_size, ici::vec64fof(vec)->v_size));
            if (!r)
            {
                return 1;
            }
            error = ippsAdd_64f(ici::vec64fof(vec)->v_ptr, ici::vec64fof(rhs)->v_ptr, r->v_ptr, int(r->v_size));
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
    return check_error(error, [result]() { return ici::ret_with_decref(result); });
}

// ----------------------------------------------------------------

//
// FFT
//

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

    void fwd(ici::vec32f *src, ici::vec32f *dst)
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

/**
 * fft = ipp.fft(order, flag, hint)
 */
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

/**
 * ipp.fwd(fft, src, dst)
 */
int f_fft_fwd()
{
    fft *f;
    ici::vec32f *src;
    ici::vec32f *dst;
    if (ici::typecheck("ooo", &f, &src, &dst))
    {
        return 1;
    }
    if (!isfft(f))
    {
        return ici::argerror(0);
    }
    if (!ici::isvec32f(src))
    {
        return ici::argerror(1);
    }
    if (!ici::isvec32f(dst))
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
        ICI_DEFINE_CFUNC(cosh,          f_cosh),
        ICI_DEFINE_CFUNC(sinh,          f_sinh),
        ICI_DEFINE_CFUNC(tanh,          f_tanh),
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

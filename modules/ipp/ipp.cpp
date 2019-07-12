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
        ICI_DEFINE_CFUNC(zero, f_zero),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(ipp));
}

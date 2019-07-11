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
        ICI_DEFINE_CFUNC(init, f_init),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(ipp));
}

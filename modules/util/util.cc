#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include "util.h"

extern "C" ici::object * ici_util_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "util"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(util)
    {
        ICI_DEFINE_CFUNC(getdata, f_util_getdata),
        ICI_DEFINE_CFUNC(pick, f_util_pick),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(util));
}

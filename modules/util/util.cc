#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

extern int f_util_getdata();

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
    ICI_DEFINE_CFUNCS(util)
    {
        ICI_DEFINE_CFUNC(getdata, f_util_getdata),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(util));
}

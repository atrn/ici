#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

extern int f_util_getdata();
extern int f_util_pick();

namespace
{

ici::cfunc *cfuncs()
{
    static ICI_DEFINE_CFUNCS(util)
    {
        ICI_DEFINE_CFUNC(getdata, f_util_getdata),
        ICI_DEFINE_CFUNC(pick, f_util_pick),
        ICI_CFUNCS_END()
    };
    return ICI_CFUNCS(util);
}

} // anon

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
    return ici::new_module(cfuncs());
}

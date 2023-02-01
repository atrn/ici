#include <ici.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

/**
 * int = vm.hash(object)
 *
 * Return the object's internal hash value.
 */
int f_hash()
{
    ici::object *o;

    if (ici::typecheck("o", &o))
    {
        return 1;
    }
    return ici::int_ret(o->hash());
}

} // namespace

extern "C" ici::object *ici_vm_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "vm"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(vm)
    {
        ICI_DEFINE_CFUNC(hash, f_hash),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(vm));
}

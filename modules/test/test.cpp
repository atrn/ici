#define ICI_MODULE_NAME test

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include <cstdio>

int f_func1(void)
{
    char *s;
    if (ici::typecheck("s", &s))
    {
        return 1;
    }
    printf("%s\n", s);
    return ici::null_ret();
}

extern "C" ici::object *ici_test_init()
{
    if
    (
        ici::check_interface(ici::version_number, ici::back_compat_version, "test")
        ||
        init_ici_str()
    )
    {
        return nullptr;
    }
    ICI_DEFINE_CFUNCS(test)
    {
        ICI_DEFINE_CFUNC(func1, f_func1),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(test));
}

#include <ici.h>

namespace
{

#include "icistr.h"
#include <icistr-setup.h>

int f_pick()
{
    ici::str    *s;
    ici::array  *index;

    if (ici::typecheck("oa", &s, &index))
    {
        return 1;
    }
    if (!ici::isstring(s))
    {
        return ici::argerror(0);
    }

    const auto n = index->len();
    const auto limit = s->s_nchars;
    auto r = ici::str_alloc(n);
    for (size_t i = 0; i < n; ++i)
    {
        auto o = index->get(i);
        if (!isint(o))
        {
            return ici::set_error("index element %llu is not an integer", static_cast<unsigned long long>(i));
        }
        const int64_t j = intof(o)->i_value;
        if (j < 0 || j >= int64_t(limit))
        {
            return ici::set_error("index element %llu with value %lld is out of bound", static_cast<unsigned long long>(i), j);
        }
        r->s_chars[i] = s->s_chars[j];
    }
    return ret_with_decref(r);
}

} // namespace

extern "C" ici::object *ici_str_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "str"))
    {
        return nullptr;
    }
    if (init_ici_str())
    {
        return nullptr;
    }
    static ICI_DEFINE_CFUNCS(str)
    {
        ICI_DEFINE_CFUNC(pick, f_pick),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(str));
}

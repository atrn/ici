#define ICI_CORE
#include "fwd.h"
#include "userop.h"
#include "map.h"
#include "null.h"
#include "int.h"
#include "cfunc.h"
#include "func.h"
#include "str.h"

namespace ici
{

map *userops = nullptr;

static str *make_key(const char *t1, const char *binop, const char *t2)
{
    char buf[100]; // really 30 _ max(binop_name) + 30
    const int n = snprintf(buf, sizeof buf, "%s %s %s", t1, binop, t2);
    return new_str(buf, n);
}

int define_user_binop(const char *t1, const char *binop, const char *t2, func *fn)
{
    if (!userops)
    {
        if (!(userops = new_map()))
        {
            return 1;
        }
    }

    ref<str> k = make_key(t1, binop, t2);
    return userops->assign(k, fn);
}

func *lookup_user_binop(const char *t1, const char *binop, const char *t2)
{
    if (!userops)
    {
        return nullptr;
    }
    ref<str> k = make_key(t1, binop, t2);
    if (auto r = userops->fetch(k))
    {
        if (isnull(r))
        {
            return nullptr;
        }
        if (isfunc(r))
        {
            return funcof(r);
        }
        set_error("%*s is %s, not a string", k->s_nchars,  k->s_chars, r->icitype()->name);
    }
    return nullptr;
}

object *call_user_binop(func *fn, object *lhs, object *rhs)
{
    object *o;
    if (call(fn, "o=oo", &o, lhs, rhs))
    {
        o = nullptr;
    }
    return o;
}

int f_binop()
{
    char *t1, *binop, *t2;
    func *fn;

    if (typecheck("ssso", &t1, &binop, &t2, &fn))
    {
        return 1;
    }
    if (!isfunc(fn))
    {
        return argerror(3);
    }
    if (define_user_binop(t1, binop, t2, fn))
    {
        return 1;
    }
    return null_ret();
}

ICI_DEFINE_CFUNCS(userop)
{
    ICI_DEFINE_CFUNC(binop, f_binop),
    ICI_CFUNCS_END()
};

} // namespace ici

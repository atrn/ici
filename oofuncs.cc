#define NOCLASSPROTO

#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "struct.h"
#include "buf.h"
#include "re.h"
#include "null.h"
#include "op.h"
#include "array.h"
#include "method.h"

namespace ici
{

/*
 * Return 0 if o (the subject object argument supplied to C implemented
 * methods) is present (indicating a method call was made) and is an
 * object with a super and, (if tcode != ICI_TC_NONE) has the given type
 * code. Else return 1 and set error appropriately.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_method_check(object *o, int tcode)
{
    char        n1[30];
    char        n2[30];

    if (o == NULL)
    {
        return ici_set_error("attempt to call method %s as a function",
            ici_objname(n1, ici_os.a_top[-1]));
    }
    if (tcode != 0 && o->o_tcode != tcode)
    {
        return ici_set_error("attempt to apply method %s to %s",
            ici_objname(n1, ici_os.a_top[-1]),
            ici_objname(n2, o));
    }
    return 0;
}

/*
 * The ICI 'new' method.
 */
static int
m_new(object *o)
{
    ici_struct *s;

    if (ici_method_check(o, 0))
        return 1;
    if ((s = ici_struct_new()) == NULL)
        return 1;
    s->o_super = objwsupof(o);
    return ici_ret_with_decref(s);
}

static int
m_isa(object *o)
{
    objwsup  *s;
    object   *klass;

    if (ici_method_check(o, 0))
        return 1;
    if (typecheck("o", &klass))
        return 1;
    for (s = objwsupof(o); s != NULL; s = s->o_super)
    {
        if (s == klass)
            return ici_ret_no_decref(ici_one);
    }
    return ici_ret_no_decref(ici_zero);
}

static int
m_respondsto(object *o)
{
    object   *classname;
    object   *v;

    if (ici_method_check(o, 0))
        return 1;
    if (typecheck("o", &classname))
        return 1;
    if ((v = ici_fetch(o, classname)) == NULL)
        return 1;
    if (ismethod(v))
    {
        return ici_ret_no_decref(v);
    }
    if (isfunc(v))
    {
        return ici_ret_with_decref(ici_method_new(o, v));
    }
    return ici_null_ret();
}

static int
m_unknown_method(object *o)
{
    if (ici_method_check(o, 0))
	return 1;
    if (NARGS() > 0 && isstring(ARG(0)))
    {
        auto name = stringof(ARG(0));
        return ici_set_error("attempt to call unknown method \"%s\"", name->s_chars);
    }
    return ici_set_error("attempt to call unknown method");
}

ICI_DEFINE_CFUNCS(oo)
{
    ICI_DEFINE_METHOD(new,              m_new),
    ICI_DEFINE_METHOD(isa,              m_isa),
    ICI_DEFINE_METHOD(respondsto,       m_respondsto),
    ICI_DEFINE_METHOD(unknown_method,   m_unknown_method),
    ICI_CFUNCS_END()
};

} // namespace ici

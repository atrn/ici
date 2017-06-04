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

/*
 * Return 0 if o (the subject object argument supplied to C implemented
 * methods) is present (indicating a method call was made) and is an
 * object with a super and, (if tcode != ICI_TC_NONE) has the given type
 * code. Else return 1 and set error appropriately.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_method_check(ici_obj_t *o, int tcode)
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
 * Implemantation of the ICI new method.
 */
static int
m_new(ici_obj_t *o)
{
    ici_struct_t    *s;

    if (ici_method_check(o, 0))
        return 1;
    if ((s = ici_struct_new()) == NULL)
        return 1;
    s->o_head.o_super = ici_objwsupof(o);
    return ici_ret_with_decref(ici_objof(s));
}

static int
m_isa(ici_obj_t *o)
{
    ici_objwsup_t   *s;
    ici_obj_t   *klass;

    if (ici_method_check(o, 0))
        return 1;
    if (ici_typecheck("o", &klass))
        return 1;
    for (s = ici_objwsupof(o); s != NULL; s = s->o_super)
    {
        if (ici_objof(s) == klass)
            return ici_ret_no_decref(ici_objof(ici_one));
    }
    return ici_ret_no_decref(ici_objof(ici_zero));
}

static int
m_respondsto(ici_obj_t *o)
{
    ici_obj_t   *classname;
    ici_obj_t   *v;

    if (ici_method_check(o, 0))
        return 1;
    if (ici_typecheck("o", &classname))
        return 1;
    if ((v = ici_fetch(o, classname)) == NULL)
        return 1;
    if (ici_ismethod(v))
    {
        return ici_ret_no_decref(v);
    }
    if (ici_isfunc(v))
    {
        return ici_ret_with_decref(ici_objof(ici_method_new(o, v)));
    }
    return ici_null_ret();
}

static int
m_unknown_method(ici_obj_t *o)
{

    if (ici_method_check(o, 0))
	return 1;
    if (ICI_NARGS() > 0 && ici_isstring(ICI_ARG(0)))
    {
        ici_str_t *name = ici_stringof(ICI_ARG(0));
        return ici_set_error("attempt to call unknown method \"%s\"", name->s_chars);
    }
    return ici_set_error("attempt to call unknown method");
}

ici_cfunc_t ici_oo_funcs[] =
{
    {ICI_CF_OBJ,    (char *)SS(new),          m_new},
    {ICI_CF_OBJ,    (char *)SS(isa),          m_isa},
    {ICI_CF_OBJ,    (char *)SS(respondsto),   m_respondsto},
    {ICI_CF_OBJ,    (char *)SS(unknown_method), m_unknown_method},
    {ICI_CF_OBJ}
};

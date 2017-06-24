#define ICI_CORE
#include "fwd.h"
#include "method.h"
#include "exec.h"
#include "buf.h"
#include "null.h"
#include "primes.h"
#include "str.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

namespace ici
{

/*
 * Returns a new ICI method object that combines the given 'subject' object
 * (typically a struct) with the given 'callable' object (typically a
 * function).  A method is also a callable object.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
method *ici_method_new(object *subject, object *callable)
{
    method   *m;

    if ((m = ici_talloc(method)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(m, ICI_TC_METHOD, 0, 1, 0);
    m->m_subject = subject;
    m->m_callable = callable;
    ici_rego(m);
    return m;
}

size_t method_type::mark(object *o)
{
    auto m = methodof(o);
    return setmark(m) + ici_mark(m->m_subject) + ici_mark(m->m_callable);
}

object * method_type::fetch(object *o, object *k)
{
    auto m = methodof(o);
    if (k == SS(subject))
        return m->m_subject;
    if (k == SS(callable))
        return m->m_callable;
    return ici_null;
}

int method_type::call(object *o, object *)
{
    auto m = methodof(o);
    if (!m->m_callable->can_call())
    {
        char    n1[ICI_OBJNAMEZ];
        char    n2[ICI_OBJNAMEZ];

        return ici_set_error("attempt to call %s:%s",
                             ici_objname(n1, m->m_subject),
                             ici_objname(n2, m->m_callable));
    }
    return m->m_callable->call(m->m_subject);
}

void method_type::objname(object *o, char p[ICI_OBJNAMEZ])
{
    char    n1[ICI_OBJNAMEZ];
    char    n2[ICI_OBJNAMEZ];

    ici_objname(n1, methodof(o)->m_subject);
    ici_objname(n2, methodof(o)->m_callable);
    sprintf(p, "(%.13s:%.13s)", n1, n2);
}

} // namespace ici

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
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
method *new_method(object *subject, object *callable)
{
    method   *m;

    if ((m = ici_talloc(method)) == nullptr)
        return nullptr;
    set_tfnz(m, TC_METHOD, 0, 1, 0);
    m->m_subject = subject;
    m->m_callable = callable;
    rego(m);
    return m;
}

size_t method_type::mark(object *o)
{
    auto m = methodof(o);
    return type::mark(m) + ici_mark(m->m_subject) + ici_mark(m->m_callable);
}

object * method_type::fetch(object *o, object *k)
{
    auto m = methodof(o);
    if (k == SS(subject))
        return m->m_subject;
    if (k == SS(callable))
        return m->m_callable;
    return null;
}

int method_type::call(object *o, object *)
{
    auto m = methodof(o);
    if (!m->m_callable->can_call())
    {
        char    n1[objnamez];
        char    n2[objnamez];

        return set_error
        (
            "attempt to call %s:%s",
            ici::objname(n1, m->m_subject),
            ici::objname(n2, m->m_callable)
        );
    }
    return m->m_callable->call(m->m_subject);
}

void method_type::objname(object *o, char p[objnamez])
{
    char    n1[objnamez];
    char    n2[objnamez];

    ici::objname(n1, methodof(o)->m_subject);
    ici::objname(n2, methodof(o)->m_callable);
    sprintf(p, "(%.13s:%.13s)", n1, n2);
}

} // namespace ici

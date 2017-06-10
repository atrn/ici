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

static void
objname_method(ici_obj_t *o, char p[ICI_OBJNAMEZ])
{
    char    n1[ICI_OBJNAMEZ];
    char    n2[ICI_OBJNAMEZ];

    ici_objname(n1, ici_methodof(o)->m_subject);
    ici_objname(n2, ici_methodof(o)->m_callable);
    sprintf(p, "(%.13s:%.13s)", n1, n2);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_method(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return ici_mark(ici_methodof(o)->m_subject)
        + ici_mark(ici_methodof(o)->m_callable);
}

/*
 * Returns a new ICI method object that combines the given 'subject' object
 * (typically a struct) with the given 'callable' object (typically a
 * function).  A method is also a callable object.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_method_t *
ici_method_new(ici_obj_t *subject, ici_obj_t *callable)
{
    ici_method_t   *m;

    if ((m = ici_talloc(ici_method_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(m, ICI_TC_METHOD, 0, 1, 0);
    m->m_subject = subject;
    m->m_callable = callable;
    ici_rego(m);
    return m;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
static void
free_method(ici_obj_t *o)
{
    ici_tfree(o, ici_method_t);
}

/*
 * Return the object which at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_method(ici_obj_t *o, ici_obj_t *k)
{
    ici_method_t        *m;

    m = ici_methodof(o);
    if (k == SSO(subject))
        return m->m_subject;
    if (k == SSO(callable))
        return m->m_callable;
    return ici_null;
}

static int
call_method(ici_obj_t *o, ici_obj_t *subject)
{
    ici_method_t        *m;

    m = ici_methodof(o);
    if (ici_typeof(m->m_callable)->t_call == NULL)
    {
        char    n1[ICI_OBJNAMEZ];
        char    n2[ICI_OBJNAMEZ];

        return ici_set_error("attempt to call %s:%s",
            ici_objname(n1, m->m_subject),
            ici_objname(n2, m->m_callable));
    }
    return (*ici_typeof(m->m_callable)->t_call)(m->m_callable, m->m_subject);
}

ici_type_t  ici_method_type =
{
    mark_method,
    free_method,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    fetch_method,
    "method",
    objname_method,
    call_method
};

} // namespace ici

#define ICI_CORE
#include "exec.h"
#include "ptr.h"
#include "map.h"
#include "int.h"
#include "op.h"
#include "buf.h"
#include "primes.h"
#include "cfunc.h"
#include "archiver.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t ptr_type::mark(object *o)
{
    auto p = ptrof(o);
    return type::mark(p) + ici_mark(p->p_aggr) + ici_mark(p->p_key);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int ptr_type::cmp(object *o1, object *o2)
{
    return ptrof(o1)->p_aggr != ptrof(o2)->p_aggr
        || ptrof(o1)->p_key != ptrof(o2)->p_key;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long ptr_type::hash(object *o)
{
    return (unsigned long)ptrof(o)->p_aggr * PTR_PRIME_0
        + (unsigned long)ptrof(o)->p_key * PTR_PRIME_1;
}

/*
 * Return the object at key k of the obejct o, or nullptr on error.
 * See the comment on t_fetch in object.h.
 *
 * Fetching a "sub element" of a pointer is the operation of pointer indexing.
 * It works for any pointer whoes key into its aggregate is an integer, and
 * of course the key must be an integer too.  The final key is the sum of the
 * two keys.  But if the key is zero, just do *ptr.
 */
object * ptr_type::fetch(object *o, object *k)
{
    if (k == o_zero)
        return ici_fetch(ptrof(o)->p_aggr, ptrof(o)->p_key);
    if (!isint(k) || !isint(ptrof(o)->p_key))
        return fetch_fail(o, k);
    if (ptrof(o)->p_key == o_zero)
        incref(k);
    else if ((k = new_int(intof(k)->i_value + intof(ptrof(o)->p_key)->i_value)) == nullptr)
        return nullptr;
    o = ici_fetch(ptrof(o)->p_aggr, k);
    decref(k);
    return o;
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * See above comment.
 */
int ptr_type::assign(object *o, object *k, object *v)
{
    if (k == o_zero)
        return ici_assign(ptrof(o)->p_aggr, ptrof(o)->p_key, v);
    if (!isint(k) || !isint(ptrof(o)->p_key))
        return assign_fail(o, k, v);
    if (ptrof(o)->p_key == o_zero)
        incref(k);
    else if ((k = new_int(intof(k)->i_value + intof(ptrof(o)->p_key)->i_value)) == nullptr)
        return 1;
    if (ici_assign(ptrof(o)->p_aggr, k, v)) {
        decref(k);
        return 1;
    }
    decref(k);
    return 0;
}

int ptr_type::call(object *o, object *)
{
    object   *f;

    if ((f = ici_fetch(ptrof(o)->p_aggr, ptrof(o)->p_key)) == nullptr)
        return 1;
    if (!f->can_call()) {
        char    n1[objnamez];
        return set_error("attempt to call a ptr pointing to %s", ici::objname(n1, o));
    }
    /*
     * Replace ourselves on the operand stack with 'self' (our aggr) and
     * push on the new object being called.
     */
    if ((os.a_top[-1] = new_int(NARGS() + 1)) == nullptr)
        return 1;
    decref((os.a_top[-1]));
    os.a_top[-2] = ptrof(o)->p_aggr;
    if (os.push_checked(f))
        return 1;
    xs.a_top[-1] = &o_call;
    /*
     * Then behave as if the target had been called. Should this do the
     * debug hooks? Assume not for now.
     */
    return f->call(nullptr);
}

/*
 * Return a new ICI pointer object. The pointer will point to the element
 * keyed by 'k' in the object 'a'.
 *
 * The returned object has had it' reference count incremented.
 *
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ptr *new_ptr(object *a, object *k)
{
    ptr *p;

    if ((p = ici_talloc(ptr)) == nullptr)
        return nullptr;
    set_tfnz(p, TC_PTR, 0, 1, 0);
    p->p_aggr = a;
    p->p_key = k;
    rego(p);
    return p;
}
/*
 * aggr key => ptr
 */
int op_mkptr()
{
    object  *o;

    if ((o = new_ptr(os.a_top[-2], os.a_top[-1])) == nullptr)
        return 1;
    os.a_top[-2] = o;
    decref(o);
    --os.a_top;
    --xs.a_top;
    return 0;
}

/*
 * ptr => aggr key
 */
int op_openptr()
{
    ptr  *p;
    char n[objnamez];

    if (!isptr(p = ptrof(os.a_top[-1])))
    {
        return set_error("pointer required, but %s given", objname(n, os.a_top[-1]));
    }
    os.a_top[-1] = p->p_aggr;
    os.push(p->p_key);
    --xs.a_top;
    return 0;
}

/*
 * ptr => obj
 */
int op_fetch()
{
    ptr  *p;
    object  *o;
    char    n[objnamez];

    if (!isptr(p = ptrof(os.a_top[-1])))
    {
        return set_error("pointer required, but %s given", objname(n, os.a_top[-1]));
    }
    if ((o = ici_fetch(p->p_aggr, p->p_key)) == nullptr)
        return 1;
    os.a_top[-1] = o;
    --xs.a_top;
    return 0;
}

int ptr_type::save(archiver *ar, object *o) {
    return ar->save(ptrof(o)->p_aggr) || ar->save(ptrof(o)->p_key);
}

object *ptr_type::restore(archiver *ar) {
    object *aggr;
    object *key;
    ptr *ptr;

    if ((aggr = ar->restore()) == nullptr) {
        return nullptr;
    }
    if ((key = ar->restore()) == nullptr) {
        decref(aggr);
        return nullptr;
    }
    ptr = new_ptr(aggr, key);
    decref(aggr);
    decref(key);
    return ptr ? ptr : nullptr;
}

op    o_mkptr{op_mkptr};
op    o_openptr{op_openptr};
op    o_fetch{op_fetch};

} // namespace ici

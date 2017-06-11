#define ICI_CORE
#include "exec.h"
#include "ptr.h"
#include "struct.h"
#include "int.h"
#include "op.h"
#include "buf.h"
#include "primes.h"
#include "cfunc.h"
#include "types.h"

namespace ici
{

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long ptr_type::mark(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof(ici_ptr_t) + ici_mark(ici_ptrof(o)->p_aggr) + ici_mark(ici_ptrof(o)->p_key);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int ptr_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    return ici_ptrof(o1)->p_aggr != ici_ptrof(o2)->p_aggr
        || ici_ptrof(o1)->p_key != ici_ptrof(o2)->p_key;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long ptr_type::hash(ici_obj_t *o)
{
    return (unsigned long)ici_ptrof(o)->p_aggr * PTR_PRIME_0
        + (unsigned long)ici_ptrof(o)->p_key * PTR_PRIME_1;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 *
 * Fetching a "sub element" of a pointer is the operation of pointer indexing.
 * It works for any pointer whoes key into its aggregate is an integer, and
 * of course the key must be an integer too.  The final key is the sum of the
 * two keys.  But if the key is zero, just do *ptr.
 */
ici_obj_t * ptr_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    if (k == ici_zero)
        return ici_fetch(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key);
    if (!ici_isint(k) || !ici_isint(ici_ptrof(o)->p_key))
        return ici_fetch_fail(o, k);
    if (ici_ptrof(o)->p_key == ici_zero)
        ici_incref(k);
    else if ((k = ici_int_new(ici_intof(k)->i_value + ici_intof(ici_ptrof(o)->p_key)->i_value))
            == NULL)
        return NULL;
    o = ici_fetch(ici_ptrof(o)->p_aggr, k);
    ici_decref(k);
    return o;
}

/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * See above comment.
 */
int ptr_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    if (k == ici_zero)
        return ici_assign(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key, v);
    if (!ici_isint(k) || !ici_isint(ici_ptrof(o)->p_key))
        return ici_assign_fail(o, k, v);
    if (ici_ptrof(o)->p_key == ici_zero)
        ici_incref(k);
    else if ((k = ici_int_new(ici_intof(k)->i_value + ici_intof(ici_ptrof(o)->p_key)->i_value))
            == NULL)
        return 1;
    if (ici_assign(ici_ptrof(o)->p_aggr, k, v))
    {
        ici_decref(k);
        return 1;
    }
    ici_decref(k);
    return 0;
}

int ptr_type::call(ici_obj_t *o, ici_obj_t *subject)
{
    ici_obj_t   *f;

    if ((f = ici_fetch(ici_ptrof(o)->p_aggr, ici_ptrof(o)->p_key)) == NULL)
        return 1;
    if (!ici_typeof(f)->can_call())
    {
        char    n1[30];
        return ici_set_error("attempt to call a ptr pointing to %s", ici_objname(n1, o));
    }
    /*
     * Replace ourselves on the operand stack with 'self' (our aggr) and
     * push on the new object being called.
     */
    if ((ici_os.a_top[-1] = ici_int_new(ICI_NARGS() + 1)) == NULL)
        return 1;
    ici_decref(ici_os.a_top[-1]);
    ici_os.a_top[-2] = ici_ptrof(o)->p_aggr;
    if (ici_stk_push_chk(&ici_os, 1))
        return 1;
    *ici_os.a_top++ = f;
    ici_xs.a_top[-1] = &ici_o_call;
    /*
     * Then behave as if the target had been called. Should this do the
     * debug hooks? Assume not for now.
     */
    return ici_typeof(f)->call(f, NULL);
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void ptr_type::free(ici_obj_t *o)
{
    ici_tfree(o, ici_ptr_t);
}

/*
 * Return a new ICI pointer object. The pointer will point to the element
 * keyed by 'k' in the object 'a'.
 *
 * The returned object has had it' reference count incremented.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_ptr_t *
ici_ptr_new(ici_obj_t *a, ici_obj_t *k)
{
    ici_ptr_t  *p;

    if ((p = ici_talloc(ici_ptr_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(p, ICI_TC_PTR, 0, 1, 0);
    p->p_aggr = a;
    p->p_key = k;
    ici_rego(p);
    return p;
}
/*
 * aggr key => ptr
 */
int
ici_op_mkptr()
{
    ici_obj_t  *o;

    if ((o = ici_ptr_new(ici_os.a_top[-2], ici_os.a_top[-1])) == NULL)
        return 1;
    ici_os.a_top[-2] = o;
    ici_decref(o);
    --ici_os.a_top;
    --ici_xs.a_top;
    return 0;
}

/*
 * ptr => aggr key
 */
int
ici_op_openptr()
{
    ici_ptr_t  *p;
    char                n[30];

    if (!ici_isptr(p = ici_ptrof(ici_os.a_top[-1])))
    {
        return ici_set_error("pointer required, but %s given", ici_objname(n, ici_os.a_top[-1]));
    }
    ici_os.a_top[-1] = p->p_aggr;
    *ici_os.a_top++ = p->p_key;
    --ici_xs.a_top;
    return 0;
}

/*
 * ptr => obj
 */
int
ici_op_fetch()
{
    ici_ptr_t  *p;
    ici_obj_t  *o;
    char                n[30];

    if (!ici_isptr(p = ici_ptrof(ici_os.a_top[-1])))
    {
        return ici_set_error("pointer required, but %s given", ici_objname(n, ici_os.a_top[-1]));
    }
    if ((o = ici_fetch(p->p_aggr, p->p_key)) == NULL)
        return 1;
    ici_os.a_top[-1] = o;
    --ici_xs.a_top;
    return 0;
}

ici_op_t    ici_o_mkptr         = {ici_op_mkptr};
ici_op_t    ici_o_openptr       = {ici_op_openptr};
ici_op_t    ici_o_fetch         = {ici_op_fetch};

} // namespace ici

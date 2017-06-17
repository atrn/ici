/*
 * array.c - implementation of the intrinsic array type. See also array.h.
 */
#define ICI_CORE
#include "fwd.h"
#include "ptr.h"
#include "exec.h"
#include "forall.h"
#include "op.h"
#include "int.h"
#include "null.h"
#include "buf.h"
#include "primes.h"

namespace ici
{

/*
 * Function to do the hard work for the inline function ici_stk_push_chk().
 * See array.h. This reallocates the array buffer.
 */
int array::grow_stack(ptrdiff_t n)
{
    ici_obj_t  **e;
    ptrdiff_t  oldz;

    /*
     * Users of arrays as stacks are supposed to know the origin and
     * history of the array. So we just assert it is not an atom.
     */
    assert(!isatom());
    assert(a_bot == a_base);

    /*
     * We don't use realloc to ensure that memory exhaustion is
     * cleanly recovereable.
     */
    if ((oldz = a_limit - a_base) * 3 / 2 < a_limit - a_base + n)
    {
        n += (a_limit - a_base) + 10;
    }
    else
    {
        n = (a_limit - a_base) * 3 / 2;
    }
    if ((e = (ici_obj_t **)ici_nalloc(n * sizeof(ici_obj_t *))) == NULL)
    {
        return 1;
    }
    memcpy((char *)e, (char *)a_base, (a_limit - a_base) * sizeof (object *));
    a_top = e + (a_top - a_base);
    ici_nfree((char *)a_base, oldz * sizeof (object *));
    a_base = e;
    a_bot = e;
    a_limit = e + n;
    return 0;
}

/*
 * Function to do the hard work for the inline function ici_stack_probe(). See array.h.
 */
int
array::fault_stack(ptrdiff_t i)
{
    /*
     * Users of arrays as stacks are supposed to know the origin and
     * history of the array. So we just assert it is not an atom.
     */
    assert(!isatom());
    ++i;
    i -= a_top - a_bot;
    if (stk_push_chk(i))
    {
        return 1;
    }
    while (--i >= 0)
    {
        *a_top++ = ici_null;
    }
    return 0;
}

/*
 * Return the number of elements in the array 'a'.
 *
 * This --func-- forms part of the --ici-api--.
 */
ptrdiff_t
array::len()
{
    if (a_top >= a_bot)
    {
        return a_top - a_bot;
    }
    return (a_top - a_base) + (a_limit - a_bot);
}

/*
 * Return a pointer to the first object in the array at index i, and
 * reduce *np to the number of contiguous elements available from that
 * point onwards. np may be NULL if not required.
 *
 * This is the commonest routine for finding an element at a given
 * index in an array. It only works for valid indexes.
 */
ici_obj_t **
array::span(int i, ptrdiff_t *np)
{
    ici_obj_t           **e;
    ptrdiff_t           n;

    if (a_bot <= a_top)
    {
        e = &a_bot[i];
        n = a_top - e;
    }
    else if (a_bot + i < a_limit)
    {
        e = &a_bot[i];
        n = a_limit - e;
    }
    else
    {
        i -= a_limit - a_bot;
        e = &a_base[i];
        n = a_top - e;
    }
    assert(n >= 0);
    if (np != NULL && *np > n)
    {
        *np = n;
    }
    return e;
}

/*
 * Copy 'n' object pointers from the given array, starting at index 'start',
 * to 'b'.  The span must cover existing elements of the array (that is, don't
 * try to read from negative or excessive indexes).
 *
 * This function is used to copy objects out of an array into a contiguous
 * destination area.  You can't easily just memcpy, because the span of
 * elements you want may wrap around the end.  For example, the implementaion
 * of interval() uses this to copy the span of elements it wants into a new
 * array.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
array::gather(ici_obj_t **b, ptrdiff_t start, ptrdiff_t n)
{
    ici_obj_t           **e;
    ptrdiff_t           i;
    ptrdiff_t           m;

    for (i = start; i < start + n; i += m, b += m)
    {
        m = n - (i - start);
        e = span(i, &m);
        memcpy(b, e, m * sizeof (object *));
    }
}

/*
 * Grow the given array to have a larger allocation. Also ensure that
 * on return there is at least one empty slot after a_top and before
 * a_bot.
 */
int array::grow()
{
    ptrdiff_t           nel;    /* Number of elements. */
    ptrdiff_t           n;      /* Old allocation count. */
    ptrdiff_t           m;      /* New allocation count. */
    ici_obj_t           **e;    /* New allocation. */

    n = a_limit - a_base;
    if ((m = n * 3 / 2) < 8)
    {
        m = 8;
    }
    if ((e = (ici_obj_t **)ici_nalloc(m * sizeof(object *))) == NULL)
    {
        return 1;
    }
    nel = len();
    gather(e + 1, 0, nel);
    ici_nfree(a_base, n * sizeof(object *));
    a_base = e;
    a_limit = e + m;
    a_bot = e + 1;
    a_top = e + 1 + nel;
    return 0;
}

/*
 * Push the object 'o' onto the end of the array 'a'. This is the general
 * case that works for any array whether it is a stack or a queue.
 * On return, o_top[-1] is the object pushed. Returns 1 on error, else 0,
 * usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
array::push(ici_obj_t *o)
{
    if (isatom())
    {
        return ici_set_error("attempt to push atomic array");
    }
    if (a_bot <= a_top)
    {
        /*
         *   ..........oooooooooooooooooooX............
         *   ^a_base   ^a_bot             ^a_top       ^a_limit
         */
        if (a_top == a_limit)
        {
            /*
             * The a_top pointer is at the limit of the array. So it has to
             * wrap to the base. But will there be room after that?
             */
            if (a_base + 1 >= a_bot)
            {
                if (grow())
                {
                    return 1;
                }
            }
            else
            {
                a_top = a_base; /* Wrap from limit to base. */
                if (a_bot == a_limit)
                {
                    a_bot = a_base; /* a_bot was also at limit. */
                }
            }
        }
    }
    else
    {
        /*
         *   ooooooooooooooX................ooooooooooo
         *   ^a_base       ^a_top           ^a_bot     ^a_limit
         */
        if (a_top + 1 >= a_bot)
        {
            if (grow())
            {
                return 1;
            }
        }
    }
    assert(a_base <= a_top);
    assert(a_top <= a_limit);
    *a_top++ = o;
    return 0;
}

/*
 * Push the object 'o' onto the front of the array 'a'. Return 1 on failure,
 * else 0, usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
array::rpush(ici_obj_t *o)
{
    if (isatom())
    {
        return ici_set_error("attempt to rpush atomic array");
    }
    if (a_bot <= a_top)
    {
        /*
         *   ..........oooooooooooooooooooX............
         *   ^a_base   ^a_bot             ^a_top       ^a_limit
         */
        if (a_bot == a_base)
        {
            if (a_top >= a_limit - 1 || a_top == a_bot)
            {
                if (grow())
                {
                    return 1;
                }
            }
            else
            {
                a_bot = a_limit; /* Wrap from base to limit. */
            }
        }
    }
    else
    {
        /*
         *   ooooooooooooooX................ooooooooooo
         *   ^a_base       ^a_top           ^a_bot     ^a_limit
         */
        if (a_top >= a_bot - 1)
        {
            if (grow())
            {
                return 1;
            }
        }
    }
    assert(a_base <= a_bot);
    assert(a_bot <= a_limit);
    *--a_bot = o;
    return 0;
}

/*
 * Pop and return the top of the given array, or 'ici_null' if it is empty.
 * Returns NULL on error (for example, attempting to pop and atomic array).
 * Usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t *
array::pop()
{
    if (isatom())
    {
        ici_set_error("attempt to pop atomic array");
        return NULL;
    }
    if (a_bot <= a_top)
    {
        /*
         *   ..........oooooooooooooooooooX............
         *   ^a_base   ^a_bot             ^a_top       ^a_limit
         */
        if (a_bot < a_top)
        {
            return *--a_top;
        }
    }
    else
    {
        /*
         *   ooooooooooooooX................ooooooooooo
         *   ^a_base       ^a_top           ^a_bot     ^a_limit
         */
        if (a_top > a_base)
        {
            return *--a_top;
        }
        a_top = a_limit;
        if (a_top > a_bot)
        {
            return *--a_top;
        }
    }
    assert(a_base <= a_top);
    assert(a_top <= a_limit);
    return ici_null;
}

/*
 * Pop and return the front of the given array, or 'ici_null' if it is empty.
 * Returns NULL on error (for example, attempting to pop and atomic array).
 * Usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t *
array::rpop()
{
    if (isatom())
    {
        ici_set_error("attempt to rpop atomic array");
        return NULL;
    }
    if (a_bot <= a_top)
    {
        /*
         *   ..........oooooooooooooooooooX............
         *   ^a_base   ^a_bot             ^a_top       ^a_limit
         */
        if (a_bot < a_top)
        {
            return *a_bot++;
        }
    }
    else
    {
        /*
         *   ooooooooooooooX................ooooooooooo
         *   ^a_base       ^a_top           ^a_bot     ^a_limit
         */
        if (a_bot < a_limit)
        {
            return *a_bot++;
        }
        a_bot = a_base;
        if (a_bot < a_top)
        {
            return *a_bot++;
        }
    }
    assert(a_base <= a_bot);
    assert(a_bot <= a_limit);
    return ici_null;
}

/*
 * Return a pointer to the slot in the array 'a' that does, or should contain
 * the index 'i'.  This will grow and 'ici_null' fill the array as necessary
 * (and fail if the array is atomic).  Only positive 'i'.  Returns NULL on
 * error, usual conventions.  This will not fail if 'i' is less than
 * 'ici_array_nels(a)'.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t **
array::find_slot(ptrdiff_t i)
{
    ptrdiff_t n;

    n = len();
    if (i < n)
    {
        /*
         * Within the range of exisiting objects. Just use
         * span to find the pointer to it.
         */
        return span(i, NULL);
    }
    if (isatom())
    {
        ici_set_error("attempt to modify an atomic array");
        return NULL;
    }
    i = i - n + 1; /* Number of elements we need to add. */
    while (--i >= 0)
    {
        if (push(ici_null))
        {
            return NULL;
        }
    }
    return &a_top[-1];
}

/*
 * Return the element or the array 'a' from index 'i', or 'ici_null' if out of
 * range.  No incref is done on the object.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t *
array::get(ptrdiff_t i)
{
    ptrdiff_t n;

    n = len();
    if (i >= 0 && i < n)
    {
        return *span(i, NULL);
    }
    return ici_null;
}

/*
 * Return a new array.  It will have room for at least 'n' elements to be
 * pushed contigously (that is, there is no need to use ici_stk_push_chk() for
 * objects pushed immediately, up to that limit).  If 'n' is 0 an internal
 * default will be used.  The returned array has ref count 1.  Returns NULL on
 * failure, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_array_t *
ici_array_new(ptrdiff_t n)
{
    ici_array_t    *a;

    if ((a = ici_talloc(ici_array_t)) == NULL)
    {
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(a, ICI_TC_ARRAY, 0, 1, 0);
    a->a_base = NULL;
    a->a_top = NULL;
    a->a_limit = NULL;
    a->a_bot = NULL;
    if (n == 0)
    {
        n = 4;
    }
    if ((a->a_base = (ici_obj_t **)ici_nalloc(n * sizeof(ici_obj_t *))) == NULL)
    {
        ici_tfree(a, ici_array_t);
        return NULL;
    }
    a->a_top = a->a_base;
    a->a_bot = a->a_base;
    a->a_limit = a->a_base + n;
    ici_rego(a);
    return a;
}

/*
 * obj => array 0 (the array contains the obj)
 */
int
ici_op_mklvalue()
{
    ici_array_t *a;

    if ((a = ici_array_new(1)) == NULL)
    {
        return 1;
    }
    *a->a_top++ = ici_os.a_top[-1];
    ici_os.a_top[-1] = a;
    *ici_os.a_top++ = ici_zero;
    a->decref();
    --ici_xs.a_top;
    return 0;
}

// array_type

unsigned long array_type::mark(ici_obj_t *o) {
    ici_obj_t           **e;

    o->setmark();
    if (ici_arrayof(o)->a_base == NULL)
    {
        return typesize();
    }
    auto mem = typesize() + (ici_arrayof(o)->a_limit - ici_arrayof(o)->a_base) * sizeof(ici_obj_t *);
    if (ici_arrayof(o)->a_bot <= ici_arrayof(o)->a_top)
    {
        for (e = ici_arrayof(o)->a_bot; e < ici_arrayof(o)->a_top; ++e)
        {
            mem += ici_mark(*e);
        }
    }
    else
    {
        for (e = ici_arrayof(o)->a_base; e < ici_arrayof(o)->a_top; ++e)
        {
            mem += ici_mark(*e);
        }
        for (e = ici_arrayof(o)->a_bot; e < ici_arrayof(o)->a_limit; ++e)
        {
            mem += ici_mark(*e);
        }
    }
    return mem;
}

void array_type::free(ici_obj_t *o)
{
    if (ici_arrayof(o)->a_base != NULL)
    {
        ici_nfree
        (
            ici_arrayof(o)->a_base,
            (ici_arrayof(o)->a_limit - ici_arrayof(o)->a_base) * sizeof(ici_obj_t *)
        );
    }
    ici_tfree(o, ici_array_t);
}

unsigned long array_type::hash(ici_obj_t *o)
{
    unsigned long       h;
    ici_obj_t           **e;
    ptrdiff_t           n;
    ptrdiff_t           m;
    ptrdiff_t           i;

    h = ARRAY_PRIME;
    n = ici_arrayof(o)->len();
    for (i = 0; i < n; )
    {
        m = n;
        e = ici_arrayof(o)->span(i, &m);
        i += m;
        while (--m >= 0)
        {
            h += ICI_PTR_HASH(*e);
            ++e;
            h >>= 1;
        }
    }
    return h;
}

int array_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    ptrdiff_t           i;
    ptrdiff_t           n1;
    ptrdiff_t           n2;
    ici_obj_t           **e1;
    ici_obj_t           **e2;

    if (o1 == o2)
    {
        return 0;
    }
    n1 = ici_arrayof(o1)->len();
    n2 = ici_arrayof(o2)->len();
    if (n1 != n2)
    {
        return 1;
    }
    for (i = 0; i < n1; i += n2)
    {
        n2 = n1;
        e1 = ici_arrayof(o1)->span(i, &n2);
        e2 = ici_arrayof(o2)->span(i, &n2);
        if (memcmp(e1, e2, n2 * sizeof(ici_obj_t *)))
        {
            return 1;
        }
    }
    return 0;
}

ici_obj_t * array_type::copy(ici_obj_t *o)
{
    ici_array_t         *na;
    ptrdiff_t           n;

    n = ici_arrayof(o)->len();
    if ((na = ici_array_new(n)) == NULL)
    {
        return NULL;
    }
    ici_arrayof(o)->gather(na->a_top, 0, n);
    na->a_top += n;
    return na;
}

int array_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    long        i;
    ici_obj_t   **e;

    if (o->isatom())
    {
        return ici_set_error("attempt to assign to an atomic array");
    }
    if (!ici_isint(k))
    {
        return assign_fail(o, k, v);
    }
    i = ici_intof(k)->i_value;
    if (i < 0)
    {
        return ici_set_error("attempt to assign to negative array index");
    }
    if ((e = ici_arrayof(o)->find_slot(i)) == NULL)
    {
        return 1;
    }
    *e = v;
    return 0;
}

ici_obj_t *array_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    if (!ici_isint(k))
    {
        return fetch_fail(o, k);
    }
    return ici_arrayof(o)->get(ici_intof(k)->i_value);
}

int array_type::forall(ici_obj_t *o)
{
    ici_forall_t *fa = forallof(o);
    ici_array_t    *a;
    ici_int_t  *i;

    a = ici_arrayof(fa->fa_aggr);
    if (++fa->fa_index >= a->len())
        return -1;
    if (fa->fa_vaggr != ici_null)
    {
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, a->get(fa->fa_index)))
            return 1;
    }
    if (fa->fa_kaggr != ici_null)
    {
        if ((i = ici_int_new((long)fa->fa_index)) == NULL)
            return 1;
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
            return 1;
        i->decref();
    }
    return 0;
}

ici_op_t    ici_o_mklvalue{ici_op_mklvalue};

} // namespace ici

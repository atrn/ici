/*
 * array.c - implementation of the intrinsic array type. See also array.h.
 */
#define ICI_CORE
#include "archiver.h"
#include "buf.h"
#include "exec.h"
#include "forall.h"
#include "fwd.h"
#include "int.h"
#include "null.h"
#include "op.h"
#include "primes.h"
#include "ptr.h"

namespace ici
{

/*
 * Function to do the hard work for the inline function push_check().
 * See array.h. This reallocates the array buffer.
 */
int array::grow_stack(ptrdiff_t n)
{
    object  **e;
    ptrdiff_t oldz;

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
    if ((e = (object **)ici_nalloc(n * sizeof(object *))) == nullptr)
    {
        return 1;
    }
    memcpy((char *)e, (char *)a_base, (a_limit - a_base) * sizeof(object *));
    a_top = e + (a_top - a_base);
    ici_nfree((char *)a_base, oldz * sizeof(object *));
    a_base = e;
    a_bot = e;
    a_limit = e + n;
    return 0;
}

/*
 * Function to do the hard work for the inline function stack_probe(). See array.h.
 */
int array::fault_stack(ptrdiff_t i)
{
    /*
     * Users of arrays as stacks are supposed to know the origin and
     * history of the array. So we just assert it is not an atom.
     */
    assert(!isatom());
    ++i;
    i -= a_top - a_bot;
    if (push_check(i))
    {
        return 1;
    }
    while (--i >= 0)
    {
        push(null);
    }
    return 0;
}

/*
 * Return the number of elements in the array 'a'.
 *
 * This --func-- forms part of the --ici-api--.
 */
size_t array::len()
{
    if (a_top >= a_bot)
    {
        return size_t(a_top - a_bot);
    }
    return size_t((a_top - a_base) + (a_limit - a_bot));
}

/*
 * Return a pointer to the first object in the array at index i, and
 * reduce *np to the number of contiguous elements available from that
 * point onwards. np may be nullptr if not required.
 *
 * This is the commonest routine for finding an element at a given
 * index in an array. It only works for valid indexes.
 */
object **array::span(size_t i, ptrdiff_t *np)
{
    object  **e;
    ptrdiff_t n;

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
    if (np != nullptr && *np > n)
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
void array::gather(object **b, ptrdiff_t start, ptrdiff_t n)
{
    object  **e;
    ptrdiff_t i;
    ptrdiff_t m;

    for (i = start; i < start + n; i += m, b += m)
    {
        m = n - (i - start);
        e = span(i, &m);
        memcpy(b, e, m * sizeof(object *));
    }
}

/*
 * Grow the given array to have a larger allocation. Also ensure that
 * on return there is at least one empty slot after a_top and before
 * a_bot.
 */
int array::grow()
{
    ptrdiff_t nel; /* Number of elements. */
    ptrdiff_t n;   /* Old allocation count. */
    ptrdiff_t m;   /* New allocation count. */
    object  **e;   /* New allocation. */

    n = a_limit - a_base;
    if ((m = n * 3 / 2) < 8)
    {
        m = 8;
    }
    if ((e = (object **)ici_nalloc(m * sizeof(object *))) == nullptr)
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
int array::push_back(object *o)
{
    if (isatom())
    {
        return set_error("attempt to push atomic array");
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
    push(o);
    return 0;
}

/*
 * Push the object 'o' onto the front of the array 'a'. Return 1 on failure,
 * else 0, usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
int array::push_front(object *o)
{
    if (isatom())
    {
        return set_error("attempt to rpush atomic array");
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
 * Pop and return the top of the given array, or 'null' if it is empty.
 * Returns nullptr on error (for example, attempting to pop and atomic array).
 * Usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
object *array::pop_back()
{
    if (isatom())
    {
        set_error("attempt to pop atomic array");
        return nullptr;
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
    return null;
}

/*
 * Pop and return the front of the given array, or 'null' if it is empty.
 * Returns nullptr on error (for example, attempting to pop and atomic array).
 * Usual error conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
object *array::pop_front()
{
    if (isatom())
    {
        set_error("attempt to rpop atomic array");
        return nullptr;
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
    return null;
}

/*
 * Return a pointer to the slot in the array 'a' that does, or should contain
 * the index 'i'.  This will grow and 'null' fill the array as necessary
 * (and fail if the array is atomic).  Only positive 'i'.  Returns nullptr on
 * error, usual conventions.  This will not fail if 'i' is less than 'len()'.
 *
 * This --func-- forms part of the --ici-api--.
 */
object **array::find_slot(ptrdiff_t i)
{
    ptrdiff_t n;

    n = len();
    if (i < n)
    {
        /*
         * Within the range of exisiting objects. Just use
         * span to find the pointer to it.
         */
        return span(i, nullptr);
    }
    if (isatom())
    {
        set_error("attempt to modify an atomic array");
        return nullptr;
    }
    i = i - n + 1; /* Number of elements we need to add. */
    while (--i >= 0)
    {
        if (push_back(null))
        {
            return nullptr;
        }
    }
    return &a_top[-1];
}

/*
 * Return the element or the array 'a' from index 'i', or 'null' if out of
 * range.  No incref is done on the object.
 *
 * This --func-- forms part of the --ici-api--.
 */
object *array::get(ptrdiff_t i)
{
    ptrdiff_t n;

    n = len();
    if (i >= 0 && i < n)
    {
        return *span(i, nullptr);
    }
    return null;
}

/*
 * Return a new array.  It will have room for at least 'n' elements to be
 * pushed contigously (that is, there is no need to use push_check() for
 * objects pushed immediately, up to that limit).  If 'n' is 0 an internal
 * default will be used.  The returned array has ref count 1.  Returns nullptr on
 * failure, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
array *new_array(ptrdiff_t n)
{
    array *a;

    if ((a = ici_talloc(array)) == nullptr)
    {
        return nullptr;
    }
    set_tfnz(a, TC_ARRAY, 0, 1, 0);
    a->a_base = nullptr;
    a->a_top = nullptr;
    a->a_limit = nullptr;
    a->a_bot = nullptr;
    if (n == 0)
    {
        n = 8; // initial capacity
    }
    if ((a->a_base = (object **)ici_nalloc(n * sizeof(object *))) == nullptr)
    {
        ici_tfree(a, array);
        return nullptr;
    }
    a->a_top = a->a_base;
    a->a_bot = a->a_base;
    a->a_limit = a->a_base + n;
    rego(a);
    return a;
}

/*
 * obj => array 0 (the array contains the obj)
 */
int op_mklvalue()
{
    ref<array> a;

    if ((a = new_array(1)) == nullptr)
    {
        return 1;
    }
    a->push(os.a_top[-1]);
    os.a_top[-1] = a;
    os.push(o_zero);
    --xs.a_top;
    return 0;
}

// array_type

size_t array_type::mark(object *o)
{
    auto a = arrayof(o);
    auto mem = type::mark(a);
    if (a->a_base == nullptr)
    {
        return mem;
    }
    mem += (a->a_limit - a->a_base) * sizeof(object *);
    if (a->a_bot <= a->a_top)
    {
        for (object **e = a->a_bot; e < a->a_top; ++e)
        {
            mem += ici_mark(*e);
        }
    }
    else
    {
        for (object **e = a->a_base; e < a->a_top; ++e)
        {
            mem += ici_mark(*e);
        }
        for (object **e = a->a_bot; e < a->a_limit; ++e)
        {
            mem += ici_mark(*e);
        }
    }
    return mem;
}

void array_type::free(object *o)
{
    auto a = arrayof(o);
    if (a->a_base != nullptr)
    {
        ici_nfree(a->a_base, (a->a_limit - a->a_base) * sizeof(object *));
    }
    ici_tfree(o, array);
}

unsigned long array_type::hash(object *o)
{
    unsigned long h;
    object      **e;
    ptrdiff_t     n;
    ptrdiff_t     m;
    ptrdiff_t     i;

    h = ARRAY_PRIME;
    n = arrayof(o)->len();
    for (i = 0; i < n;)
    {
        m = n;
        e = arrayof(o)->span(i, &m);
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

int array_type::cmp(object *o1, object *o2)
{
    ptrdiff_t i;
    ptrdiff_t n1;
    ptrdiff_t n2;
    object  **e1;
    object  **e2;

    if (o1 == o2)
    {
        return 0;
    }
    n1 = arrayof(o1)->len();
    n2 = arrayof(o2)->len();
    if (n1 != n2)
    {
        return 1;
    }
    for (i = 0; i < n1; i += n2)
    {
        n2 = n1;
        e1 = arrayof(o1)->span(i, &n2);
        e2 = arrayof(o2)->span(i, &n2);
        if (memcmp(e1, e2, n2 * sizeof(object *)))
        {
            return 1;
        }
    }
    return 0;
}

object *array_type::copy(object *o)
{
    array    *na;
    ptrdiff_t n;

    n = arrayof(o)->len();
    if ((na = new_array(n)) == nullptr)
    {
        return nullptr;
    }
    arrayof(o)->gather(na->a_top, 0, n);
    na->a_top += n;
    return na;
}

int array_type::assign(object *o, object *k, object *v)
{
    int64_t  i;
    object **e;

    if (o->isatom())
    {
        return set_error("attempt to assign to an atomic array");
    }
    if (!isint(k))
    {
        return assign_fail(o, k, v);
    }
    i = intof(k)->i_value;
    if (i < 0)
    {
        i += arrayof(o)->len();
    }
    if ((e = arrayof(o)->find_slot(i)) == nullptr)
    {
        return 1;
    }
    *e = v;
    return 0;
}

object *array_type::fetch(object *o, object *k)
{
    if (!isint(k))
    {
        return fetch_fail(o, k);
    }
    if (intof(k)->i_value >= 0)
    {
        return arrayof(o)->get(intof(k)->i_value);
    }
    auto idx = intof(k)->i_value + arrayof(o)->len();
    return arrayof(o)->get(idx);
}

int array_type::forall(object *o)
{
    auto     fa = forallof(o);
    array   *a;
    integer *i;

    a = arrayof(fa->fa_aggr);
    if (++fa->fa_index >= a->len())
    {
        return -1;
    }
    if (fa->fa_vaggr != null)
    {
        if (ici_assign(fa->fa_vaggr, fa->fa_vkey, a->get(fa->fa_index)))
        {
            return 1;
        }
    }
    if (fa->fa_kaggr != null)
    {
        if ((i = make_ref(new_int((long)fa->fa_index))) == nullptr)
        {
            return 1;
        }
        if (ici_assign(fa->fa_kaggr, fa->fa_kkey, i))
        {
            return 1;
        }
    }
    return 0;
}

int array_type::save(archiver *ar, object *o)
{
    auto a = arrayof(o);
    if (ar->save_name(o))
    {
        return 1;
    }
    const int64_t len = a->len();
    if (ar->write(len))
    {
        return 1;
    }
    for (object **e = a->astart(); e != a->alimit(); e = a->anext(e))
    {
        if (ar->save(*e))
        {
            return 1;
        }
    }
    return 0;
}

object *array_type::restore(archiver *ar)
{
    int64_t    len;
    ref<array> a;
    object    *name;

    if (ar->restore_name(&name))
    {
        return nullptr;
    }
    if (ar->read(&len))
    {
        return nullptr;
    }
    if (len < 0)
    {
        set_error("restored invalid array lemgth");
        return nullptr;
    }
    if ((a = make_ref(new_array(len))) == nullptr)
    {
        return nullptr;
    }
    if (ar->record(name, a))
    {
        return nullptr;
    }
    for (; len > 0; --len)
    {
        ref<> o;

        if ((o = make_ref(ar->restore())) == nullptr)
        {
            goto fail;
        }
        if (a->push_back(o))
        {
            goto fail;
        }
    }
    return a.release();

fail:
    ar->remove(name);
    return nullptr;
}

int64_t array_type::len(object *o)
{
    return arrayof(o)->len();
}

op o_mklvalue{op_mklvalue};

} // namespace ici

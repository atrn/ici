#define ICI_CORE
#include "object.h"
#include "set.h"
#include "exec.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "null.h"
#include "primes.h"
#include "forall.h"

namespace ici
{

inline size_t hashindex(object *k, set *s)
{
    return ICI_PTR_HASH(k) & (s->s_nslots - 1);
}

/*
 * Find the set slot which does, or should, contain the key k.
 */
object **ici_find_set_slot(set *s, object *k)
{
    object  **e;

    e = &s->s_slots[hashindex(k, s)];
    while (*e != NULL)
    {
        if (*e == k)
            return e;
        if (--e < s->s_slots)
            e = s->s_slots + s->s_nslots - 1;
    }
    return e;
}

/*
 * Return a new ICI set object. The returned set has been increfed.
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
set *new_set()
{
    set  *s;

    /*
     * NB: there is a copy of this sequence in copy_set.
     */
    if ((s = ici_talloc(set)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, ICI_TC_SET, 0, 1, 0);
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (object **)ici_nalloc(4 * sizeof (object *))) == NULL)
    {
        ici_tfree(s, set);
        return NULL;
    }
    memset(s->s_slots, 0, 4 * sizeof (object *));
    ici_rego(s);
    return s;
}

/*
 * Grow the set s so that it has twice as many slots.
 */
static int grow_set(set *s)
{
    object    **e;
    object    **oldslots;
    size_t     z;
    ptrdiff_t   oldn;

    oldn = s->s_nslots;
    z = (oldn * 2) * sizeof (object *);
    if ((e = (object **)ici_nalloc(z)) == NULL)
        return 1;
    memset((char *)e, 0, z);
    oldslots = s->s_slots;
    s->s_slots = e;
    s->s_nslots = oldn * 2;
    for (z = oldn; z > 0; )
    {
        --z;
        if (oldslots[z] != NULL)
            *ici_find_set_slot(s, oldslots[z]) = oldslots[z];
    }
    ici_nfree(oldslots, oldn * sizeof (object *));
    return 0;
}

/*
 * Remove the key from the set.
 */
int unassign(set *s, object *k)
{
    object  **sl;
    object  **ss;
    object  **ws;   /* Wanted position. */

    if (*(ss = ici_find_set_slot(s, k)) == NULL)
        return 0;
    --s->s_nels;
    sl = ss;
    /*
     * Scan "forward" bubbling up entries which would rather be at our
     * current empty slot.
     */
    for (;;)
    {
        if (--sl < s->s_slots)
            sl = s->s_slots + s->s_nslots - 1;
        if (*sl == NULL)
            break;
        ws = &s->s_slots[hashindex(*sl, s)];
        if
        (
            (sl < ss && (ws >= ss || ws < sl))
            ||
            (sl > ss && (ws >= ss && ws < sl))
        )
        {
            /*
             * The value at sl, which really wants to be at ws, should go
             * into the current empty slot (ss).  Copy it to there and update
             * ss to be here (which now becomes empty).
             */
            *ss = *sl;
            ss = sl;
        }
    }
    *ss = NULL;
    return 0;
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t set_type::mark(object *o)
{
    auto s = setof(o);
    auto mem = setmark(s) + s->s_nslots * sizeof (object *);
    if (s->s_nels == 0)
        return mem;
    for (object **e = &s->s_slots[s->s_nslots - 1]; e >= s->s_slots; --e)
        mem += maybe_mark(*e);
    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void set_type::free(object *o)
{
    auto s = setof(o);
    if (s->s_slots != NULL)
        ici_nfree(s->s_slots, s->s_nslots * sizeof (object *));
    ici_tfree(o, set);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int set_type::cmp(object *o1, object *o2)
{
    object **e;

    if (o1 == o2)
        return 0;
    if (setof(o1)->s_nels != setof(o2)->s_nels)
        return 1;
    e = setof(o1)->s_slots;
    for (auto i = setof(o1)->s_nslots; i-- > 0; )
    {
        if (*e != NULL && *ici_find_set_slot(setof(o2), *e) == NULL)
            return 1;
        ++e;
    }
    return 0;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long set_type::hash(object *o)
{
    unsigned long   h;
    object        **po;

    /*
     * This assumes NULL will become zero when cast to unsigned long.
     */

    h = 0;
    po = setof(o)->s_slots;
    for (auto i = setof(o)->s_nels; i-- > 0; )
        h += (unsigned long)*po++ >> 4;
    return h * SET_PRIME_0 + SET_PRIME_1;
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
object * set_type::copy(object *o)
{
    set   *s;
    set   *ns;

    s = setof(o);
    if ((ns = ici_talloc(set)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(ns, ICI_TC_SET, 0, 1, 0);
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ici_rego(ns);
    if ((ns->s_slots = (object **)ici_nalloc(s->s_nslots * sizeof (object *))) == NULL)
        goto fail;
    memcpy(ns->s_slots, s->s_slots, s->s_nslots * sizeof (object *));
    ns->s_nels = s->s_nels;
    ns->s_nslots = s->s_nslots;
    return ns;

fail:
    ns->decref();
    return NULL;
}


/*
 * Assign to key k of the object o the value v. Return 1 on error, else 0.
 * See the comment on t_assign() in object.h.
 *
 * Add or delete the key k from the set based on the value of v.
 */
int set_type::assign(object *o, object *k, object *v)
{
    auto s = setof(o);
    object  **e;

    if (o->isatom())
    {
        return set_error("attempt to modify an atomic set");
    }
    if (isfalse(v))
    {
        return unassign(s, k);
    }
    if (*(e = ici_find_set_slot(s, k)) != NULL)
        return 0;
    if (s->s_nels >= s->s_nslots - s->s_nslots / 4)
    {
        /*
         * This set is 75% full.  Grow it.
         */
        if (grow_set(s))
            return 1;
        e = ici_find_set_slot(s, k);
    }
    ++s->s_nels;
    *e = k;
    return 0;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
object * set_type::fetch(object *o, object *k)
{
    auto slot = *ici_find_set_slot(setof(o), k);
    if (slot == NULL) {
        return ici_null;
    }
    return o_one;
}

int set_type::forall(object *o)
{
    auto fa = forallof(o);
    set           *s;
    object       **sl;

    s = setof(fa->fa_aggr);
    while (++fa->fa_index < s->s_nslots)
    {
        if (*(sl = &s->s_slots[fa->fa_index]) == NULL)
        {
            continue;
        }
        if (fa->fa_kaggr == ici_null)
        {
            if (fa->fa_vaggr != ici_null)
            {
                if (ici_assign(fa->fa_vaggr, fa->fa_vkey, *sl))
                {
                    return 1;
                }
            }
        }
        else
        {
            if (fa->fa_vaggr != ici_null)
            {
                if (ici_assign(fa->fa_vaggr, fa->fa_vkey, o_one))
                    return 1;
            }
            if (ici_assign(fa->fa_kaggr, fa->fa_kkey, *sl))
            {
                return 1;
            }
        }
        return 0;
    }
    return -1;
}

/*
 * Return 1 if a is a subset of b, else 0.
 */
int set_issubset(set *a, set *b) /* a is a subset of b */
{
    object  **sl;
    size_t  i;

    for (sl = a->s_slots, i = 0; i < a->s_nslots; ++i, ++sl)
    {
        if (*sl == NULL)
            continue;
        if (*ici_find_set_slot(b, *sl) == NULL)
            return 0;
    }
    return 1;
}

/*
 * Return 1 if a is a proper subset of b, else 0. That is, is a subset
 * and not equal.
 */
int set_ispropersubset(set *a, set *b) /* a is a proper subset of b */
{
    return a->s_nels < b->s_nels && set_issubset(a, b);
}

} // namespace ici

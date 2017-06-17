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

#define SET_HASHINDEX(k, s) (ICI_PTR_HASH(k) & ((s)->s_nslots - 1))

namespace ici
{

/*
 * Find the set slot which does, or should, contain the key k.
 */
ici_obj_t **
ici_find_set_slot(ici_set_t *s, ici_obj_t *k)
{
    ici_obj_t  **e;

    e = &s->s_slots[SET_HASHINDEX(k, s)];
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
ici_set_t *
ici_set_new()
{
    ici_set_t  *s;

    /*
     * NB: there is a copy of this sequence in copy_set.
     */
    if ((s = ici_talloc(ici_set_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, ICI_TC_SET, 0, 1, 0);
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (ici_obj_t **)ici_nalloc(4 * sizeof(ici_obj_t *))) == NULL)
    {
        ici_tfree(s, ici_set_t);
        return NULL;
    }
    memset(s->s_slots, 0, 4 * sizeof(ici_obj_t *));
    ici_rego(s);
    return s;
}

/*
 * Grow the set s so that it has twice as many slots.
 */
static int
grow_set(ici_set_t *s)
{
    ici_obj_t           **e;
    ici_obj_t           **oldslots;
    int                 i;
    ptrdiff_t           oldn;

    oldn = s->s_nslots;
    i = (oldn * 2) * sizeof(ici_obj_t *);
    if ((e = (ici_obj_t **)ici_nalloc(i)) == NULL)
        return 1;
    memset((char *)e, 0, i);
    oldslots = s->s_slots;
    s->s_slots = e;
    s->s_nslots = oldn * 2;
    i = oldn;
    while (--i >= 0)
    {
        if (oldslots[i] != NULL)
            *ici_find_set_slot(s, oldslots[i]) = oldslots[i];
    }
    ici_nfree(oldslots, oldn * sizeof(ici_obj_t *));
    return 0;
}

/*
 * Remove the key from the set.
 */
int
ici_set_unassign(ici_set_t *s, ici_obj_t *k)
{
    ici_obj_t  **sl;
    ici_obj_t  **ss;
    ici_obj_t  **ws;   /* Wanted position. */

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
        ws = &s->s_slots[SET_HASHINDEX(*sl, s)];
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
unsigned long set_type::mark(ici_obj_t *o)
{
    ici_obj_t  **e;
    long                mem;

    o->o_flags |= ICI_O_MARK;
    mem = sizeof(ici_set_t) + ici_setof(o)->s_nslots * sizeof(ici_obj_t *);
    if (ici_setof(o)->s_nels == 0)
        return mem;
    for
    (
        e = &ici_setof(o)->s_slots[ici_setof(o)->s_nslots - 1];
        e >= ici_setof(o)->s_slots;
        --e
    )
    {
        if (*e != NULL)
            mem += ici_mark(*e);
    }
    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void set_type::free(ici_obj_t *o)
{
    if (ici_setof(o)->s_slots != NULL)
        ici_nfree(ici_setof(o)->s_slots, ici_setof(o)->s_nslots * sizeof(ici_obj_t *));
    ici_tfree(o, ici_set_t);
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int set_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    int        i;
    ici_obj_t  **e;

    if (o1 == o2)
        return 0;
    if (ici_setof(o1)->s_nels != ici_setof(o2)->s_nels)
        return 1;
    e = ici_setof(o1)->s_slots;
    i = ici_setof(o1)->s_nslots;
    while (--i >= 0)
    {
        if (*e != NULL && *ici_find_set_slot(ici_setof(o2), *e) == NULL)
            return 1;
        ++e;
    }
    return 0;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long set_type::hash(ici_obj_t *o)
{
    int                         i;
    unsigned long               h;
    ici_obj_t                   **po;

    h = 0;
    po = ici_setof(o)->s_slots;
    i = ici_setof(o)->s_nels;
    /*
     * This assumes NULL will become zero when cast to unsigned long.
     */
    while (--i >= 0)
        h += (unsigned long)*po++ >> 4;
    return h * SET_PRIME_0 + SET_PRIME_1;
}

/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
ici_obj_t * set_type::copy(ici_obj_t *o)
{
    ici_set_t   *s;
    ici_set_t   *ns;

    s = ici_setof(o);
    if ((ns = ici_talloc(ici_set_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(ns, ICI_TC_SET, 0, 1, 0);
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ici_rego(ns);
    if ((ns->s_slots = (ici_obj_t **)ici_nalloc(s->s_nslots * sizeof(ici_obj_t *))) == NULL)
        goto fail;
    memcpy(ns->s_slots, s->s_slots, s->s_nslots*sizeof(ici_obj_t *));
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
int set_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_obj_t  **e;

    if (o->isatom())
    {
        return ici_set_error("attempt to modify an atomic set");
    }
    if (isfalse(v))
    {
        return ici_set_unassign(ici_setof(o), k);
    }
    else
    {
        if (*(e = ici_find_set_slot(ici_setof(o), k)) != NULL)
            return 0;
        if (ici_setof(o)->s_nels >= ici_setof(o)->s_nslots - ici_setof(o)->s_nslots / 4)
        {
            /*
             * This set is 75% full.  Grow it.
             */
            if (grow_set(ici_setof(o)))
                return 1;
            e = ici_find_set_slot(ici_setof(o), k);
        }
        ++ici_setof(o)->s_nels;
        *e = k;
    }

    return 0;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
ici_obj_t * set_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    auto slot = *ici_find_set_slot(ici_setof(o), k);
    if (slot == NULL) {
        return ici_null;
    }
    return ici_one;
}

int set_type::forall(ici_obj_t *o)
{
    ici_forall_t *fa = forallof(o);
    ici_set_t  *s;
    ici_obj_t  **sl;

    s = ici_setof(fa->fa_aggr);
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
                if (ici_assign(fa->fa_vaggr, fa->fa_vkey, ici_one))
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
int
ici_set_issubset(ici_set_t *a, ici_set_t *b) /* a is a subset of b */
{
    ici_obj_t  **sl;
    int        i;

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
int
ici_set_ispropersubset(ici_set_t *a, ici_set_t *b) /* a is a proper subset of b */
{
    return a->s_nels < b->s_nels && ici_set_issubset(a, b);
}

} // namespace ici

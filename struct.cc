#define ICI_CORE
#include "fwd.h"
#include "struct.h"
#include "str.h"
#include "ptr.h"
#include "null.h"
#include "exec.h"
#include "func.h"
#include "op.h"
#include "int.h"
#include "buf.h"
#include "pc.h"
#include "primes.h"
#include "forall.h"

namespace ici
{

/*
 * Generation number of look-up look-asides.  All strings that hold a look-up
 * look-aside to shortcut struct lookups also record a generation number.
 * Whenever something happens that would invalidate such a look-aside (that we
 * can't recover from by some local operation) we bump the generation number.
 * This has the effect of globally invalidating all the current look-asides.
 *
 * This is the global generation (version) number.
 */
uint32_t        ici_vsver   = 1;

/*
 * Hash a pointer to get the initial position in a struct has table.
 */
inline size_t HASHINDEX(ici_obj_t *k, ici_struct_t *s) {
    return ICI_PTR_HASH(k) & (s->s_nslots - 1);
}

/*
 * Find the struct slot which does, or should, contain the key k.  Does
 * not look down the super chain.
 */
ici_sslot_t *
ici_find_raw_slot(ici_struct_t *s, ici_obj_t *k)
{
    ici_sslot_t *sl = &s->s_slots[HASHINDEX(k, s)];
    while (LIKELY(sl->sl_key != NULL))
    {
        if (LIKELY(sl->sl_key == k))
	{
            return sl;
	}
	--sl;
        if (UNLIKELY(sl < s->s_slots))
	{
            sl = s->s_slots + s->s_nslots - 1;
	}
    }
    return sl;
}

/*
 * Return a new ICI struct object. The returned struct has been increfed.
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_struct_t *
ici_struct_new()
{
    ici_struct_t   *s;

    /*
     * NB: there is a copy of this sequence in copy_struct.
     */
    if ((s = ici_talloc(ici_struct_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(s, ICI_TC_STRUCT, ICI_O_SUPER, 1, 0);
    s->o_super = NULL;
    s->s_slots = NULL;
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (ici_sslot_t*)ici_nalloc(4 * sizeof(ici_sslot_t))) == NULL)
    {
        ici_tfree(s, ici_struct_t);
        return NULL;
    }
    memset(s->s_slots, 0, 4 * sizeof(ici_sslot_t));
    ici_rego(s);
    return s;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
/*
 * Invalidate the lookup lookaside of any string keyed entries in
 * this struct. This can be done for small structs as an alternative
 * to ++ici_vsver which completely invalidates all lookasides. Only
 * call this if you know exactly what you are doing.
 */
void
ici_invalidate_struct_lookaside(ici_struct_t *s)
{
    ici_sslot_t     *sl;
    ici_sslot_t     *sle;
    ici_str_t       *str;

    sl = s->s_slots;
    sle = sl + s->s_nslots;
    while (sl < sle)
    {
        if (sl->sl_key != NULL && ici_isstring(sl->sl_key))
        {
            str = ici_stringof(sl->sl_key);
            /*ici_stringof(sl->sl_key)->s_vsver = 0;*/
            str->s_vsver = ici_vsver;
            str->s_struct = s;
            str->s_slot = sl;
        }
        ++sl;
    }
}


/*
 * Grow the struct s so that it has twice as many slots.
 */
static int
grow_struct(ici_struct_t *s)
{
    ici_sslot_t *sl;
    ici_sslot_t *oldslots;
    int        i;

    i = (s->s_nslots * 2) * sizeof(ici_sslot_t);
    if ((sl = (ici_sslot_t*)ici_nalloc(i)) == NULL)
        return 1;
    memset((char *)sl, 0, i);
    oldslots = s->s_slots;
    s->s_slots = sl;
    i = s->s_nslots;
    s->s_nslots *= 2;
    while (--i >= 0)
    {
        if (oldslots[i].sl_key != NULL)
	{
            *ici_find_raw_slot(s, oldslots[i].sl_key) = oldslots[i];
	}
    }
    ici_nfree((char *)oldslots, (s->s_nslots / 2) * sizeof(ici_sslot_t));
    ++ici_vsver;
    return 0;
}

/*
 * Remove the key 'k' from the ICI struct object 's', ignoring super-structs.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_struct_unassign(ici_struct_t *s, ici_obj_t *k)
{
    ici_sslot_t *sl;
    ici_sslot_t *ss;
    ici_sslot_t *ws;    /* Wanted position. */

    if ((ss = ici_find_raw_slot(s, k))->sl_key == NULL)
    {
        return;
    }
    --s->s_nels;
    sl = ss;
    /*
     * Scan "forward" bubbling up entries which would rather be at our
     * current empty slot.
     */
    for (;;)
    {
        if (--sl < s->s_slots)
        {
            sl = s->s_slots + s->s_nslots - 1;
        }
        if (sl->sl_key == NULL)
        {
            break;
        }
        ws = &s->s_slots[HASHINDEX(sl->sl_key, s)];
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
            /*
             * If we've moved a slot keyed by a string, that string's
             * look-aside value may be wrong. Trash it.
             */
            if (ici_isstring(ss->sl_key))
	    {
                ici_stringof(ss->sl_key)->s_vsver = 0;
	    }
        }
    }
    if (ici_isstring(k))
    {
        ici_stringof(k)->s_vsver = 0;
    }
    ss->sl_key = NULL;
    ss->sl_value = NULL;
}

/*
 * Do a fetch where we are the super of some other object that is
 * trying to satisfy a fetch. Don't regard the item k as being present
 * unless it really is. Return -1 on error, 0 if it was not found,
 * and 1 if was found. If found, the value is stored in *v.
 *
 * If not NULL, b is a struct that was the base element of this
 * fetch. This is used to mantain the lookup lookaside mechanism.
 */
int struct_type::fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b)
{
    ici_sslot_t         *sl;

    do
    {
        sl = &ici_structof(o)->s_slots[HASHINDEX(k, ici_structof(o))];
        while (sl->sl_key != NULL)
        {
            if (sl->sl_key == k)
            {
                if (b != NULL && ici_isstring(k))
                {
                    ici_stringof(k)->s_vsver = ici_vsver;
                    ici_stringof(k)->s_struct = b;
                    ici_stringof(k)->s_slot = sl;
                    if (o->isatom())
                    {
                        k->setflag(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                    else
                    {
                        k->clrflag(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                }
                *v = sl->sl_value;
                return 1;
            }
            if (--sl < ici_structof(o)->s_slots)
            {
                sl = ici_structof(o)->s_slots + ici_structof(o)->s_nslots - 1;
            }
        }
        if ((o = ici_structof(o)->o_super) == NULL)
        {
            return 0;
        }

    } while (ici_isstruct(o)); /* Merge tail recursion on structs. */

    return ici_fetch_super(o, k, v, b);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
unsigned long struct_type::mark(ici_obj_t *o)
{
    ici_sslot_t *sl;
    unsigned long mem;

    do /* Merge tail recursion on o_super. */
    {
        o->setmark();
        mem = typesize() + ici_structof(o)->s_nslots * sizeof(ici_sslot_t);
        if (ici_structof(o)->s_nels != 0)
        {
            for
            (
                sl = &ici_structof(o)->s_slots[ici_structof(o)->s_nslots - 1];
                sl >= ici_structof(o)->s_slots;
                --sl
            )
            {
                if (sl->sl_key != NULL)
                    mem += ici_mark(sl->sl_key);
                if (sl->sl_value != NULL)
                    mem += ici_mark(sl->sl_value);
            }
        }

    } while
        (
            (o = ici_structof(o)->o_super) != NULL
            &&
            !o->marked()
        );

    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void struct_type::free(ici_obj_t *o)
{
    if (ici_structof(o)->s_slots != NULL)
    {
        ici_nfree(ici_structof(o)->s_slots, ici_structof(o)->s_nslots * sizeof(ici_sslot_t));
    }
    ici_tfree(o, ici_struct_t);
    ++ici_vsver;
}

unsigned long struct_type::hash(ici_obj_t *o)
{
    int                         i;
    unsigned long               hk;
    unsigned long               hv;
    ici_sslot_t                 *sl;

    hk = 0;
    hv = 0;
    sl = ici_structof(o)->s_slots;
    i = ici_structof(o)->s_nels;
    /*
     * This assumes NULL will become zero when cast to unsigned long.
     */
    while (--i >= 0)
    {
        hk += (unsigned long)sl->sl_key >> 4;
        hv += (unsigned long)sl->sl_value >> 4;
        ++sl;
    }
    return hv * STRUCT_PRIME_0 + hk * STRUCT_PRIME_1 + STRUCT_PRIME_2;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int struct_type::cmp(ici_obj_t *o1, ici_obj_t *o2)
{
    int        i;
    ici_sslot_t *sl1;
    ici_sslot_t *sl2;

    if (ici_structof(o1) == ici_structof(o2))
    {
        return 0;
    }
    if (ici_structof(o1)->s_nels != ici_structof(o2)->s_nels)
    {
        return 1;
    }
    if (ici_structof(o1)->o_super != ici_structof(o2)->o_super)
    {
        return 1;
    }
    sl1 = ici_structof(o1)->s_slots;
    i = ici_structof(o1)->s_nslots;
    while (--i >= 0)
    {
        if (sl1->sl_key != NULL)
        {
            sl2 = ici_find_raw_slot(ici_structof(o2), sl1->sl_key);
            if (sl1->sl_key != sl2->sl_key || sl1->sl_value != sl2->sl_value)
            {
                return 1;
            }
        }
        ++sl1;
    }
    return 0;
}


/*
 * Return a copy of the given object, or NULL on error.
 * See the comment on t_copy() in object.h.
 */
ici_obj_t *struct_type::copy(ici_obj_t *o)
{
    ici_struct_t    *s;
    ici_struct_t    *ns;

    s = ici_structof(o);
    if ((ns = (ici_struct_t *)ici_talloc(ici_struct_t)) == NULL)
    {
        return NULL;
    }
    ICI_OBJ_SET_TFNZ(ns, ICI_TC_STRUCT, ICI_O_SUPER, 1, 0);
    ns->o_super = s->o_super;
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ns->s_slots = NULL;
    ici_rego(ns);
    if ((ns->s_slots = (ici_sslot_t*)ici_nalloc(s->s_nslots * sizeof(ici_sslot_t))) == NULL)
    {
        goto fail;
    }
    memcpy((char *)ns->s_slots, (char *)s->s_slots, s->s_nslots*sizeof(ici_sslot_t));
    ns->s_nels = s->s_nels;
    ns->s_nslots = s->s_nslots;
    if (ns->s_nslots <= 64)
    {
        ici_invalidate_struct_lookaside(ns);
    }
    else
    {
        ++ici_vsver;
    }
    return ns;

 fail:
    ns->decref();
    return NULL;
}


/*
 * Do an assignment where we are the super of some other object that
 * is trying to satisfy an assign. Don't regard the item k as being
 * present unless it really is. Return -1 on error, 0 if not found
 * and 1 if the assignment was completed.
 *
 * If 0 is returned, no struct may have been modified during the
 * operation of this function.
 *
 * If not NULL, b is a struct that was the base element of this
 * assignment. This is used to mantain the lookup lookaside mechanism.
 */
int struct_type::assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b)
{
    ici_sslot_t         *sl;

    do
    {
        if (!o->isatom())
        {
            sl = &ici_structof(o)->s_slots[HASHINDEX(k, ici_structof(o))];
            while (sl->sl_key != NULL)
            {
                if (sl->sl_key == k)
                {
                    sl->sl_value = v;
                    if (b != NULL && ici_isstring(k))
                    {
                        ici_stringof(k)->s_vsver = ici_vsver;
                        ici_stringof(k)->s_struct = b;
                        ici_stringof(k)->s_slot = sl;
                        k->clrflag(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                    return 1;
                }
                if (--sl < ici_structof(o)->s_slots)
                {
                    sl = ici_structof(o)->s_slots + ici_structof(o)->s_nslots - 1;
                }
            }
        }
        if ((o = ici_structof(o)->o_super) == NULL)
        {
            return 0;
        }

    } while (ici_isstruct(o)); /* Merge tail recursion. */

    return ici_assign_super(o, k, v, b);
}

/*
 * Set the value of key k in the struct s to the value v.  Will add the
 * entry if necessary and grow the struct if necessary.  Returns 1 on
 * failure, else 0.
 * See the comment on t_assign() in object.h.
 */
int struct_type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_sslot_t         *sl;

    if
    (
        ici_isstring(k)
        &&
        ici_stringof(k)->s_struct == ici_structof(o)
        &&
        ici_stringof(k)->s_vsver == ici_vsver
        &&
        !k->flag(ICI_S_LOOKASIDE_IS_ATOM)
    )
    {
#ifndef NDEBUG
        ici_obj_t       *av;
        assert(fetch_super(o, k, &av, NULL) == 1);
        assert(ici_stringof(k)->s_slot->sl_value == av);
#endif
        ici_stringof(k)->s_slot->sl_value = v;
        return 0;
    }
    /*
     * Look for it in the base struct.
     */
    sl = &ici_structof(o)->s_slots[HASHINDEX(k, ici_structof(o))];
    while (sl->sl_key != NULL)
    {
        if (sl->sl_key == k)
        {
            if (o->isatom())
            {
                return ici_set_error("attempt to modify an atomic struct");
            }
            goto do_assign;
        }
        if (--sl < ici_structof(o)->s_slots)
            sl = ici_structof(o)->s_slots + ici_structof(o)->s_nslots - 1;
    }
    if (ici_structof(o)->o_super != NULL)
    {
        switch (ici_assign_super(ici_structof(o)->o_super, k, v, ici_structof(o)))
        {
        case -1: return 1; /* Error. */
        case 1:  return 0; /* Done. */
        }
    }
    /*
     * Not found. Assign into base struct. We still have sl from above.
     */
    if (o->isatom())
    {
        return ici_set_error("attempt to modify an atomic struct");
    }
    if (ici_structof(o)->s_nels >= ici_structof(o)->s_nslots - ici_structof(o)->s_nslots / 4)
    {
        /*
         * This struct is 75% full.  Grow it.
         */
        if (grow_struct(ici_structof(o)))
            return 1;
        /*
         * Re-find our empty slot.
         */
        sl = &ici_structof(o)->s_slots[HASHINDEX(k, ici_structof(o))];
        while (sl->sl_key != NULL)
        {
            if (--sl < ici_structof(o)->s_slots)
                sl = ici_structof(o)->s_slots + ici_structof(o)->s_nslots - 1;
        }
    }
    ++ici_structof(o)->s_nels;
    sl->sl_key = k;
 do_assign:
    sl->sl_value = v;
    if (ici_isstring(k))
    {
        ici_stringof(k)->s_vsver = ici_vsver;
        ici_stringof(k)->s_struct = ici_structof(o);
        ici_stringof(k)->s_slot = sl;
        k->clrflag(ICI_S_LOOKASIDE_IS_ATOM);
    }
    return 0;
}

/*
 * Assign a value into a key of a struct, but ignore the super chain.
 * That is, always assign into the lowest level. Usual error coventions.
 */
int struct_type::assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_struct_t      	*s = ici_structof(o);
    ici_sslot_t         *sl;
    int                 tqfull;

    if (UNLIKELY(o->isatom()))
    {
        return ici_set_error("attempt to modify an atomic struct");
    }
    sl = ici_find_raw_slot(s, k);
    if (sl->sl_key != NULL)
    {
        goto do_assign;
    }
    /*
     * Not found. Assign into base struct. We still have sl from above.
     */
    tqfull = s->s_nels >= s->s_nslots - s->s_nslots / 4;
    if (UNLIKELY(tqfull))
    {
        /*
         * This struct is 75% full.  Grow it.
         */
        if (UNLIKELY(grow_struct(s)))
        {
            return 1;
        }
        /*
         * Re-find out empty slot.
         */
        sl = &s->s_slots[HASHINDEX(k, s)];
        while (LIKELY(sl->sl_key != NULL))
        {
            --sl;
            if (UNLIKELY(sl < s->s_slots))
            {
                sl = s->s_slots + s->s_nslots - 1;
            }
        }
    }
    ++s->s_nels;
    sl->sl_key = k;
 do_assign:
    sl->sl_value = v;
    if (LIKELY(ici_isstring(k)))
    {
        ici_stringof(k)->s_vsver = ici_vsver;
        ici_stringof(k)->s_struct = s;
        ici_stringof(k)->s_slot = sl;
        k->clrflag(ICI_S_LOOKASIDE_IS_ATOM);
    }
    return 0;
}

int struct_type::forall(ici_obj_t *o)
{
    ici_forall_t        *fa = forallof(o);
    ici_struct_t        *s  = ici_structof(fa->fa_aggr);

    while (++fa->fa_index < s->s_nslots)
    {
        ici_sslot_t     *sl = &s->s_slots[fa->fa_index];

        if (sl->sl_key == NULL)
        {
            continue;
        }
        if (fa->fa_vaggr != ici_null)
        {
            if (ici_assign(fa->fa_vaggr, fa->fa_vkey, sl->sl_value))
                return 1;
        }
        if (fa->fa_kaggr != ici_null)
        {
            if (ici_assign(fa->fa_kaggr, fa->fa_kkey, sl->sl_key))
                return 1;
        }
        return 0;
    }
    return -1;
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
ici_obj_t *struct_type::fetch(ici_obj_t *o, ici_obj_t *k)
{
    ici_obj_t           *v;

    if
    (
        ici_isstring(k)
        &&
        ici_stringof(k)->s_struct == ici_structof(o)
        &&
        ici_stringof(k)->s_vsver == ici_vsver
    )
    {
        assert(fetch_super(o, k, &v, NULL) == 1);
        assert(ici_stringof(k)->s_slot->sl_value == v);
        return ici_stringof(k)->s_slot->sl_value;
    }
    switch (fetch_super(o, k, &v, ici_structof(o)))
    {
    case -1: return NULL;               /* Error. */
    case  1: return v;                  /* Found. */
    }
    return ici_null;                    /* Not found. */
}

ici_obj_t *struct_type::fetch_base(ici_obj_t *o, ici_obj_t *k)
{
    ici_sslot_t         *sl;

    sl = ici_find_raw_slot(ici_structof(o), k);
    if (sl->sl_key == NULL)
    {
        return ici_null;
    }
    if (ici_isstring(k))
    {
        ici_stringof(k)->s_vsver = ici_vsver;
        ici_stringof(k)->s_struct = ici_structof(o);
        ici_stringof(k)->s_slot = sl;
        if (o->isatom())
        {
            k->setflag(ICI_S_LOOKASIDE_IS_ATOM);
        }
        else
        {
            k->clrflag(ICI_S_LOOKASIDE_IS_ATOM);
        }
    }
    return sl->sl_value;
}

ici_op_t    ici_o_namelvalue{ICI_OP_NAMELVALUE};
ici_op_t    ici_o_colon{ICI_OP_COLON};
ici_op_t    ici_o_coloncaret{ICI_OP_COLONCARET};
ici_op_t    ici_o_dot{ICI_OP_DOT};
ici_op_t    ici_o_dotkeep{ICI_OP_DOTKEEP};
ici_op_t    ici_o_dotrkeep{ICI_OP_DOTRKEEP};

} // namespace ici

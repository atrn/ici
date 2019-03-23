#define ICI_CORE
#include "fwd.h"
#include "map.h"
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
#include "archiver.h"

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
uint32_t        vsver   = 1;

/*
 * Hash a pointer to get the initial position in a struct has table.
 */
inline size_t hashindex(object *k, map *s) {
    return ICI_PTR_HASH(k) & (s->s_nslots - 1);
}

/*
 * Find the struct slot which does, or should, contain the key k.  Does
 * not look down the super chain.
 */
slot *find_raw_slot(map *s, object *k)
{
    slot *sl = &s->s_slots[hashindex(k, s)];
    while (LIKELY(sl->sl_key != nullptr))
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
 * Returns nullptr on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
map *new_map()
{
    map   *s;

    /*
     * NB: there is a copy of this sequence in copy_map.
     */
    if ((s = ici_talloc(map)) == nullptr)
        return nullptr;
    set_tfnz(s, TC_MAP, object::O_SUPER, 1, 0);
    s->o_super = nullptr;
    s->s_slots = nullptr;
    s->s_nels = 0;
    s->s_nslots = 4; /* Must be power of 2. */
    if ((s->s_slots = (slot*)ici_nalloc(4 * sizeof (slot))) == nullptr)
    {
        ici_tfree(s, map);
        return nullptr;
    }
    memset(s->s_slots, 0, 4 * sizeof (slot));
    rego(s);
    return s;
}

map *new_map(objwsup *super) {
    auto m = new_map();
    if (m) {
        m->o_super = super;
    }
    return m;
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
/*
 * Invalidate the lookup lookaside of any string keyed entries in
 * this struct. This can be done for small structs as an alternative
 * to ++vsver which completely invalidates all lookasides. Only
 * call this if you know exactly what you are doing.
 */
void invalidate_map_lookaside(map *s)
{
    slot     *sl;
    slot     *sle;
    str       *str;

    sl = s->s_slots;
    sle = sl + s->s_nslots;
    while (sl < sle)
    {
        if (sl->sl_key != nullptr && isstring(sl->sl_key))
        {
            str = stringof(sl->sl_key);
            /*stringof(sl->sl_key)->s_vsver = 0;*/
            str->s_vsver = vsver;
            str->s_map = s;
            str->s_slot = sl;
        }
        ++sl;
    }
}


/*
 * Grow the struct s so that it has twice as many slots.
 */
static int
grow_map(map *s)
{
    slot *sl;
    slot *oldslots;
    int   i;

    i = (s->s_nslots * 2) * sizeof (slot);
    if ((sl = (slot*)ici_nalloc(i)) == nullptr)
        return 1;
    memset((char *)sl, 0, i);
    oldslots = s->s_slots;
    s->s_slots = sl;
    i = s->s_nslots;
    s->s_nslots *= 2;
    while (--i >= 0)
    {
        if (oldslots[i].sl_key != nullptr)
        {
            *find_raw_slot(s, oldslots[i].sl_key) = oldslots[i];
        }
    }
    ici_nfree((char *)oldslots, (s->s_nslots / 2) * sizeof (slot));
    ++vsver;
    return 0;
}

/*
 * Remove the key 'k' from the ICI struct object 's', ignoring super-structs.
 *
 * This --func-- forms part of the --ici-api--.
 */
int unassign(map *s, object *k) {
    slot *sl;
    slot *ss;
    slot *ws;    /* Wanted position. */

    if ((ss = find_raw_slot(s, k))->sl_key == nullptr) {
        return 0;
    }
    --s->s_nels;
    sl = ss;
    /*
     * Scan "forward" bubbling up entries which would rather be at our
     * current empty slot.
     */
    for (;;) {
        if (--sl < s->s_slots) {
            sl = s->s_slots + s->s_nslots - 1;
        }
        if (sl->sl_key == nullptr) {
            break;
        }
        ws = &s->s_slots[hashindex(sl->sl_key, s)];
        if
        (
            (sl < ss && (ws >= ss || ws < sl))
            ||
            (sl > ss && (ws >= ss && ws < sl))
        ) {
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
            if (isstring(ss->sl_key)) {
                stringof(ss->sl_key)->s_vsver = 0;
            }
        }
    }
    if (isstring(k)) {
        stringof(k)->s_vsver = 0;
    }
    ss->sl_key = nullptr;
    ss->sl_value = nullptr;
    return 0;
}

/*
 * Do a fetch where we are the super of some other object that is
 * trying to satisfy a fetch. Don't regard the item k as being present
 * unless it really is. Return -1 on error, 0 if it was not found,
 * and 1 if was found. If found, the value is stored in *v.
 *
 * If not nullptr, b is a struct that was the base element of this
 * fetch. This is used to mantain the lookup lookaside mechanism.
 */
int map_type::fetch_super(object *o, object *k, object **v, map *b)
{
    slot *sl;

    do
    {
        sl = &mapof(o)->s_slots[hashindex(k, mapof(o))];
        while (sl->sl_key != nullptr)
        {
            if (sl->sl_key == k)
            {
                if (b != nullptr && isstring(k))
                {
                    stringof(k)->s_vsver = vsver;
                    stringof(k)->s_map = b;
                    stringof(k)->s_slot = sl;
                    if (o->isatom())
                    {
                        k->set(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                    else
                    {
                        k->clr(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                }
                *v = sl->sl_value;
                return 1;
            }
            if (--sl < mapof(o)->s_slots)
            {
                sl = mapof(o)->s_slots + mapof(o)->s_nslots - 1;
            }
        }
        if ((o = mapof(o)->o_super) == nullptr)
        {
            return 0;
        }

    } while (ismap(o)); /* Merge tail recursion on structs. */

    return ici_fetch_super(o, k, v, b);
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t map_type::mark(object *o)
{
    slot *sl;
    unsigned long mem;

    do /* Merge tail recursion on o_super. */
    {
        o->setmark();
        mem = objectsize() + mapof(o)->s_nslots * sizeof (slot);
        if (mapof(o)->s_nels != 0)
        {
            for
            (
                sl = &mapof(o)->s_slots[mapof(o)->s_nslots - 1];
                sl >= mapof(o)->s_slots;
                --sl
            )
            {
                if (sl->sl_key != nullptr)
                    mem += ici_mark(sl->sl_key);
                if (sl->sl_value != nullptr)
                    mem += ici_mark(sl->sl_value);
            }
        }

    } while
        (
            (o = mapof(o)->o_super) != nullptr
            &&
            !o->marked()
        );

    return mem;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void map_type::free(object *o)
{
    if (mapof(o)->s_slots != nullptr)
    {
        ici_nfree(mapof(o)->s_slots, mapof(o)->s_nslots * sizeof (slot));
    }
    ici_tfree(o, map);
    ++vsver;
}

unsigned long map_type::hash(object *o)
{
    int                   i;
    unsigned long         hk;
    unsigned long         hv;
    slot                 *sl;

    hk = 0;
    hv = 0;
    sl = mapof(o)->s_slots;
    i = mapof(o)->s_nels;
    /*
     * This assumes nullptr will become zero when cast to unsigned long.
     */
    while (--i >= 0)
    {
        hk += (unsigned long)sl->sl_key >> 4;
        hv += (unsigned long)sl->sl_value >> 4;
        ++sl;
    }
    return hv * MAP_PRIME_0 + hk * MAP_PRIME_1 + MAP_PRIME_2;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int map_type::cmp(object *o1, object *o2)
{
    size_t i;
    slot *sl1;
    slot *sl2;

    if (mapof(o1) == mapof(o2))
    {
        return 0;
    }
    if (mapof(o1)->s_nels != mapof(o2)->s_nels)
    {
        return 1;
    }
    if (mapof(o1)->o_super != mapof(o2)->o_super)
    {
        return 1;
    }
    sl1 = mapof(o1)->s_slots;
    i = mapof(o1)->s_nslots;
    while (i-- > 0)
    {
        if (sl1->sl_key != nullptr)
        {
            sl2 = find_raw_slot(mapof(o2), sl1->sl_key);
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
 * Return a copy of the given object, or nullptr on error.
 * See the comment on t_copy() in object.h.
 */
object *map_type::copy(object *o)
{
    map    *s;
    map    *ns;

    s = mapof(o);
    if ((ns = (map *)ici_talloc(map)) == nullptr)
    {
        return nullptr;
    }
    set_tfnz(ns, TC_MAP, object::O_SUPER, 1, 0);
    ns->o_super = s->o_super;
    ns->s_nels = 0;
    ns->s_nslots = 0;
    ns->s_slots = nullptr;
    rego(ns);
    if ((ns->s_slots = (slot*)ici_nalloc(s->s_nslots * sizeof (slot))) == nullptr)
    {
        goto fail;
    }
    memcpy((char *)ns->s_slots, (char *)s->s_slots, s->s_nslots * sizeof (slot));
    ns->s_nels = s->s_nels;
    ns->s_nslots = s->s_nslots;
    if (ns->s_nslots <= 64)
    {
        invalidate_map_lookaside(ns);
    }
    else
    {
        ++vsver;
    }
    return ns;

 fail:
    decref(ns);
    return nullptr;
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
 * If not nullptr, b is a struct that was the base element of this
 * assignment. This is used to mantain the lookup lookaside mechanism.
 */
int map_type::assign_super(object *o, object *k, object *v, map *b)
{
    slot *sl;

    do
    {
        if (!o->isatom())
        {
            sl = &mapof(o)->s_slots[hashindex(k, mapof(o))];
            while (sl->sl_key != nullptr)
            {
                if (sl->sl_key == k)
                {
                    sl->sl_value = v;
                    if (b != nullptr && isstring(k))
                    {
                        stringof(k)->s_vsver = vsver;
                        stringof(k)->s_map = b;
                        stringof(k)->s_slot = sl;
                        k->clr(ICI_S_LOOKASIDE_IS_ATOM);
                    }
                    return 1;
                }
                if (--sl < mapof(o)->s_slots)
                {
                    sl = mapof(o)->s_slots + mapof(o)->s_nslots - 1;
                }
            }
        }
        if ((o = mapof(o)->o_super) == nullptr)
        {
            return 0;
        }

    } while (ismap(o)); /* Merge tail recursion. */

    return ici_assign_super(o, k, v, b);
}

/*
 * Set the value of key k in the struct s to the value v.  Will add the
 * entry if necessary and grow the struct if necessary.  Returns 1 on
 * failure, else 0.
 * See the comment on t_assign() in object.h.
 */
int map_type::assign(object *o, object *k, object *v)
{
    slot *sl;

    if
    (
        isstring(k)
        &&
        stringof(k)->s_map == mapof(o)
        &&
        stringof(k)->s_vsver == vsver
        &&
        !k->hasflag(ICI_S_LOOKASIDE_IS_ATOM)
    )
    {
#ifndef NDEBUG
        object       *av;
        assert(fetch_super(o, k, &av, nullptr) == 1);
        assert(stringof(k)->s_slot->sl_value == av);
#endif
        stringof(k)->s_slot->sl_value = v;
        return 0;
    }
    /*
     * Look for it in the base struct.
     */
    sl = &mapof(o)->s_slots[hashindex(k, mapof(o))];
    while (sl->sl_key != nullptr)
    {
        if (sl->sl_key == k)
        {
            if (o->isatom())
            {
                return set_error("attempt to modify an atomic struct");
            }
            goto do_assign;
        }
        if (--sl < mapof(o)->s_slots)
            sl = mapof(o)->s_slots + mapof(o)->s_nslots - 1;
    }
    if (mapof(o)->o_super != nullptr)
    {
        switch (ici_assign_super(mapof(o)->o_super, k, v, mapof(o)))
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
        return set_error("attempt to modify an atomic struct");
    }
    if (mapof(o)->s_nels >= mapof(o)->s_nslots - mapof(o)->s_nslots / 4)
    {
        /*
         * This struct is 75% full.  Grow it.
         */
        if (grow_map(mapof(o)))
            return 1;
        /*
         * Re-find our empty slot.
         */
        sl = &mapof(o)->s_slots[hashindex(k, mapof(o))];
        while (sl->sl_key != nullptr)
        {
            if (--sl < mapof(o)->s_slots)
                sl = mapof(o)->s_slots + mapof(o)->s_nslots - 1;
        }
    }
    ++mapof(o)->s_nels;
    sl->sl_key = k;
 do_assign:
    sl->sl_value = v;
    if (isstring(k))
    {
        stringof(k)->s_vsver = vsver;
        stringof(k)->s_map = mapof(o);
        stringof(k)->s_slot = sl;
        k->clr(ICI_S_LOOKASIDE_IS_ATOM);
    }
    return 0;
}

/*
 * Assign a value into a key of a struct, but ignore the super chain.
 * That is, always assign into the lowest level. Usual error coventions.
 */
int map_type::assign_base(object *o, object *k, object *v)
{
    map  *s = mapof(o);
    slot       *sl;
    int         tqfull;

    if (UNLIKELY(o->isatom()))
    {
        return set_error("attempt to modify an atomic struct");
    }
    sl = find_raw_slot(s, k);
    if (sl->sl_key != nullptr)
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
        if (UNLIKELY(grow_map(s)))
        {
            return 1;
        }
        /*
         * Re-find out empty slot.
         */
        sl = &s->s_slots[hashindex(k, s)];
        while (LIKELY(sl->sl_key != nullptr))
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
    if (LIKELY(isstring(k)))
    {
        stringof(k)->s_vsver = vsver;
        stringof(k)->s_map = s;
        stringof(k)->s_slot = sl;
        k->clr(ICI_S_LOOKASIDE_IS_ATOM);
    }
    return 0;
}

int map_type::forall(object *o)
{
    struct forall *fa = forallof(o);
    map    *s  = mapof(fa->fa_aggr);

    while (++fa->fa_index < s->s_nslots)
    {
        slot *sl = &s->s_slots[fa->fa_index];

        if (sl->sl_key == nullptr)
        {
            continue;
        }
        if (fa->fa_vaggr != null)
        {
            if (ici_assign(fa->fa_vaggr, fa->fa_vkey, sl->sl_value))
                return 1;
        }
        if (fa->fa_kaggr != null)
        {
            if (ici_assign(fa->fa_kaggr, fa->fa_kkey, sl->sl_key))
                return 1;
        }
        return 0;
    }
    return -1;
}

/*
 * Return the object at key k of the obejct o, or nullptr on error.
 * See the comment on t_fetch in object.h.
 */
object *map_type::fetch(object *o, object *k)
{
    object           *v;

    if
    (
        isstring(k)
        &&
        stringof(k)->s_map == mapof(o)
        &&
        stringof(k)->s_vsver == vsver
    )
    {
        assert(fetch_super(o, k, &v, nullptr) == 1);
        assert(stringof(k)->s_slot->sl_value == v);
        return stringof(k)->s_slot->sl_value;
    }
    switch (fetch_super(o, k, &v, mapof(o)))
    {
    case -1: return nullptr;               /* Error. */
    case  1: return v;                  /* Found. */
    }
    return null;                    /* Not found. */
}

object *map_type::fetch_base(object *o, object *k)
{
    slot *sl;

    sl = find_raw_slot(mapof(o), k);
    if (sl->sl_key == nullptr)
    {
        return null;
    }
    if (isstring(k))
    {
        stringof(k)->s_vsver = vsver;
        stringof(k)->s_map = mapof(o);
        stringof(k)->s_slot = sl;
        if (o->isatom())
        {
            k->set(ICI_S_LOOKASIDE_IS_ATOM);
        }
        else
        {
            k->clr(ICI_S_LOOKASIDE_IS_ATOM);
        }
    }
    return sl->sl_value;
}

int map_type::save(archiver *ar, object *o) {
    map *s = mapof(o);
    object *super = s->o_super;

    if (super == nullptr) {
        super = null;
    }
    if (ar->save_name(o)) {
        return 1;
    }
    if (ar->save(super)) {
        return 1;
    }
    const int64_t nels = s->s_nels;
    if (ar->write(nels)) {
        return 1;
    }
    for (slot *sl = s->s_slots; size_t(sl - s->s_slots) < s->s_nslots; ++sl) {
        if (sl->sl_key && sl->sl_value) {
            auto do_pop_name = false;
            if (ismap(sl->sl_value) && isstring(sl->sl_key)) {
                ar->push_name(stringof(sl->sl_key));
                do_pop_name = true;
            }
            if (ar->save(sl->sl_key)) {
                return 1;
            }
            if (ar->save(sl->sl_value)) {
                return 1;
            }
            if (do_pop_name) {
                ar->pop_name();
            }
        }
    }
    return 0;
}

object *map_type::restore(archiver *ar) {
    map *s;
    object *super;
    int64_t n;
    object *name;

    if (ar->restore_name(&name)) {
        return nullptr;
    }
    if ((s = new_map()) == nullptr) {
        return nullptr;
    }
    if (ar->record(name, s)) {
        goto fail;
    }
    if ((super = ar->restore()) == nullptr) {
        goto fail1;
    }
    if (super != null) {
        s->o_super = objwsupof(super);
    }
    decref(super);
    if (ar->read(&n)) {
        goto fail1;
    }
    for (int64_t i = 0; i < n; ++i) {
        object *key;
        object *value;
        int failed;

        if ((key = ar->restore()) == nullptr) {
            goto fail1;
        }
        if ((value = ar->restore()) == nullptr) {
            decref(key);
            goto fail1;
        }
        failed = ici_assign(s, key, value);
        decref(key);
        decref(value);
        if (failed) {
            goto fail1;
        }
    }
    return s;

fail1:
    ar->remove(name);

fail:
    decref(s);
    return nullptr;
}

int64_t map_type::len(object *o) {
    return mapof(o)->s_nels;
}

int map_type::nkeys(object *o) {
    return mapof(o)->s_nels;
}

int map_type::keys(object *o, array *k) {
    map *s = mapof(o);
    slot *sl;
    for (sl = s->s_slots; sl < s->s_slots + s->s_nslots; ++sl) {
	if (sl->sl_key != nullptr) {
	    if (k->push_back(sl->sl_key)) {
		return 1;
	    }
	}
    }
    return 0;
}

op    o_namelvalue{OP_NAMELVALUE};
op    o_colon{OP_COLON};
op    o_coloncaret{OP_COLONCARET};
op    o_dot{OP_DOT};
op    o_dotkeep{OP_DOTKEEP};
op    o_dotrkeep{OP_DOTRKEEP};

} // namespace ici

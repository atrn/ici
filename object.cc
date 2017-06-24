#define ICI_CORE
#include "object.h"
#include "primes.h"
#include "float.h"
#include "int.h"
#include "str.h"

#include <limits.h>

namespace ici
{

/*
 * The following define is useful during debug and testing. It will
 * cause every call to an allocation function to garbage collect.
 * It will be very slow, but problems with memory will be tripped
 * over sooner. To be effective, you really also need to set
 * ICI_ALLALLOC in alloc.h.
 */
#define ALLCOLLECT      0       /* Collect on every alloc call. */

/*
 * All objects are in the objects list or completely static and
 * known to never require collection.
 */
object       **objs;         /* List of all objects. */
object       **objs_limit;   /* First element we can't use in list. */
object       **objs_top;     /* Next unused element in list. */

object       **atoms;    /* Hash table of atomic objects. */
size_t       atomsz;     /* Number of slots in hash table. */
size_t       natoms;     /* Number of atomic objects. */

int          supress_collect;
int          ncollects;	/* Number of collect() calls */

/*
 * Format a human readable version of the object 'o' into the buffer
 * 'p' in less than 30 chars. Returns 'p'. See 'The error return
 * convention' for some examples.
 *
 * This --func-- forms part of the --ici-api--.
 */
char *objname(char p[objnamez], object *o)
{
    if (o->otype()->can_objname())
    {
        o->otype()->objname(o, p);
        return p;
    }
    if (isstring(o))
    {
        if (stringof(o)->s_nchars > objnamez - 6)
            sprintf(p, "\"%.24s...\"", stringof(o)->s_chars);
        else
            sprintf(p, "\"%s\"", stringof(o)->s_chars);
    }
    else if (isint(o))
        sprintf(p, "%lld", static_cast<long long int>(intof(o)->i_value));
    else if (isfloat(o))
        sprintf(p, "%g", floatof(o)->f_value);
    else if (strchr("aeiou", o->type_name()[0]) != NULL)
        sprintf(p, "an %s", o->type_name());
    else
        sprintf(p, "a %s", o->type_name());
    return p;
}

inline unsigned long hash(object *o)
{
    if (isint(o)) return (unsigned long)intof(o)->i_value * INT_PRIME;
    return o->hash();
}

/*
 * Grow the hash table of atoms to the given size, which *must* be a
 * power of 2.
 */
static void grow_atoms_core(ptrdiff_t newz)
{
    object  **po;
    int        i;
    object           **olda;
    ptrdiff_t           oldz;

    assert(((newz - 1) & newz) == 0); /* Assert power of 2. */
    oldz = atomsz;
    ++supress_collect;
    po = (object **)ici_nalloc(newz * sizeof (object *));
    --supress_collect;
    if (po == NULL)
        return;
    atomsz = newz;
    memset((char *)po, 0, newz * sizeof (object *));
    olda = atoms;
    atoms = po;
    i = oldz;
    while (--i >= 0)
    {
        object   *o;

        if ((o = olda[i]) != NULL)
        {
            for
            (
                po = &atoms[atom_hash_index(hash(o))];
                *po != NULL;
                --po < atoms ? po = atoms + atomsz - 1 : NULL
            )
                ;
            *po = o;
        }
    }
    ici_nfree(olda, oldz * sizeof (object *));
}

/*
 * Grow the hash table of atoms to the given size, which *must* be a
 * power of 2.
 */
void grow_atoms(ptrdiff_t newz)
{
    /*
     * If there are a lot of collectable atoms, it is better for performance
     * to collect them than grow the atom pool. If we are getting close to the
     * point where we would want to collect anyway, do it and exit early if
     * we managed to reduce the usage of the atom pool alot.
     */
    if (ici_mem * 3 / 2 > ici_mem_limit)
    {
        collect();
        if (natoms * 8 < size_t(newz))
        {
            return;
        }
    }
    grow_atoms_core(newz);
}

/*
 * Return the atomic form of the given object 'o'.  This will be an object
 * equal to the one given, but read-only and possibly shared by others.  (If
 * the object it already the atomic form, it is just returned.)
 *
 * This is achieved by looking for an object of equal value in the
 * 'atom pool'. The atom pool is a hash table of all atoms. The object's
 * 't_hash' and 't_cmp' functions will be used it this lookup process
 * (from this object's 'type').
 * 
 * If an existing atomic form of the object is found in the atom pool,
 * it is returned.
 *
 * If the 'lone' flag is 1, the object is free'd if it isn't used.
 * ("lone" because the caller has the lone reference to it and will replace
 * that with what atom returns anyway.) If the 'lone' flag is zero, and the
 * object would be used (rather than returning an equal object already in the
 * atom pool), a copy will made and that copy stored in the atom pool and
 * returned.  Also note that if lone is 1 and the object is not used, the
 * nrefs of the passed object will be transfered to the object being returned.
 *
 * Never fails, at worst it just returns its argument (for historical
 * reasons).
 *
 * This --func-- forms part of the --ici-api--.
 */
object *atom(object *o, int lone)
{
    object   **po;

    assert(!(lone == 1 && o->o_nrefs == 0));

    if (o->isatom())
        return o;
    for
    (
        po = &atoms[atom_hash_index(hash(o))];
        *po != NULL;
        --po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
        if (o->o_tcode == (*po)->o_tcode && ici_cmp(o, *po) == 0)
        {
            if (lone)
            {
                (*po)->o_nrefs += o->o_nrefs;
                o->o_nrefs = 0;
            }
            return *po;
        }
    }

    /*
     * Not found.  Add this object (or a copy of it) to the atom pool.
     */
    if (!lone)
    {
        ++supress_collect;
        *po = ici_copy(o);
        --supress_collect;
        if (*po == NULL)
            return o;
        o = *po;
    }
    *po = o;
    o->set(object::O_ATOM);
    if (++natoms > atomsz / 2)
        grow_atoms(atomsz * 2);
    if (!lone)
        o->decref();
    return o;
}

/*
 * See comment on ici_atom_probe() below.
 *
 * The argument ppo, if given, and if this function returns NULL, will be
 * updated to point to the slot in the atom pool where this object belongs.
 * The caller may use this to store the new object in *provided* the atom pool
 * is not disturbed in the meantime, and is checked for possible growth
 * afterwards.  The function store_atom_and_count() can be used for this.
 * Note that any call to collect() could disturb the atom pool.
 */
object *atom_probe2(object *o, object ***ppo)
{
    object   **po;

    for
    (
        po = &atoms[atom_hash_index(hash(o))];
        *po != NULL;
        --po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
        if (o->o_tcode == (*po)->o_tcode && ici_cmp(o, *po) == 0)
            return *po;
    }
    if (ppo != NULL)
        *ppo = po;
    return NULL;
}

/*
 * Probe the atom pool for an atomic form of o.  If found, return that atomic
 * form, else NULL.  This can be use by *_new() routines of intrinsically
 * atomic objects.  These routines generally set up a dummy version of the
 * object being made which is passed to this probe.  If it finds a match, that
 * is returned, thus avoiding the allocation of an object that may be thrown
 * away anyway.
 *
 * This --func-- forms part of the --ici-api--.
 */
object *atom_probe(object *o)
{
    return atom_probe2(o, NULL);
}

/*
 * Remove an object from the atom pool. This is only done because the
 * object is being garbage collected. Even then, it is only done if there
 * are a minority of atomic objects being collected. See collect() for
 * the only call.
 *
 * Normally returns 0, but if the object could not be found in the pool,
 * returns 1. In theory this can't happen. But this return allows for a
 * little more robustness if something is screwy.
 */
static int
unatom(object *o)
{
    object  **sl;
    object  **ss;
    object  **ws;   /* Wanted position. */

    for
    (
        ss = &atoms[atom_hash_index(hash(o))];
        *ss != NULL;
        --ss < atoms ? ss = atoms + atomsz - 1 : NULL
    )
    {
        if (o == *ss)
           goto deleteo;
    }
    /*
     * The object isn't in the pool. This would seem to indicate that
     * we have been given a bad pointer, or the O_ATOM flag of some object
     * has been set spuriously.
     */
    assert(0);
    return 1;

deleteo:
    o->clr(object::O_ATOM);
    --natoms;
    sl = ss;
    /*
     * Scan "forward" bubbling up entries which would rather be at our
     * current empty slot.
     */
    for (;;)
    {
        if (--sl < atoms)
            sl = atoms + atomsz - 1;
        if (*sl == NULL)
            break;
        ws = &atoms[atom_hash_index(hash(*sl))];
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

void grow_objs(object *o)
{
    object           **newobjs;
    ptrdiff_t           newz;
    ptrdiff_t           oldz;

    oldz = objs_limit - objs;
    newz = 2 * oldz;
    ++supress_collect;
    if ((newobjs = (object **)ici_nalloc(newz * sizeof (object *))) == NULL)
    {
        --supress_collect;
        return;
    }
    --supress_collect;
    memcpy((char *)newobjs, (char *)objs, (char *)objs_limit - (char *)objs);
    objs_limit = newobjs + newz;
    objs_top = newobjs + (objs_top - objs);
    memset((char *)objs_top, 0, (char *)objs_limit - (char *)objs_top);
    ici_nfree(objs, oldz * sizeof (object *));
    objs = newobjs;
    *objs_top++ = o;
}

/*
 * Mark sweep garbage collection.  Should be safe to do any time, as new
 * objects are created without the nrefs == 0 which allows them to be
 * collected.  They must be explicitly lost before they are subject
 * to garbage collection.  But of course all code must be careful not
 * to hang on to "found" objects where they are not accessible, or they
 * will be collected.  You can ici_incref() them if you want.  All "held" objects
 * will cause all objects referenced from them to be marked (ie, not
 * collected), as long as they are registered on either the global object
 * list or in the atom pool.  Thus statically declared objects which
 * reference other objects (very rare) must be appropriately registered.
 */
void collect()
{
    object  **a;
    object  *o;
    object  **b;
    size_t  mem;    /* Total mem tied up in refed objects. */
    /*int        ndead_atoms;*/

    if (supress_collect)
    {
        /*
         * There are some times when it is a bad idea to collect. Basicly
         * when we are fiddling with the basic data structures like the
         * atom pool and recursive calls.
         */
        return;
    }
    ++supress_collect;

    ++ncollects;

#   ifndef NDEBUG
    /*
     * In debug builds we take this opportunity to check the consistency of of
     * the atom pool.  We check that each entry has the O_ATOM flag set, and
     * that it can be found in the pool (i.e.  that its hash is the same as
     * when it was inserted).  A failure here is a common result of a hash
     * and/or cmp function that considers information that changes during the
     * life of the object.
     */
    {
        object **a;

        if ((a = &atoms[atomsz]) != NULL)
        {
            while (--a >= atoms)
            {
                if (*a == NULL)
                    continue;
                assert((*a)->isatom());
                assert(atom_probe2(*a, NULL) == *a);
            }
        }
    }
#   endif

    /*
     * Mark all objects which are referenced (and thus what they ref).
     */
    mem = 0;
    for (a = objs; a < objs_top; ++a)
    {
        if ((*a)->o_nrefs != 0)
	{
            mem += ici_mark(*a);
	}
    }

#if 0
    /*
     * Count how many atoms are going to be retained and how many are
     * going to be lost so we can decide on the fastest method.
     */
    ndead_atoms = 0;
    for (a = objs; a < objs_top; ++a)
    {
        if ((*a)->flags(O_ATOM|O_MARK) == O_ATOM)
            ++ndead_atoms;
    }

    /*
     * Collection phase.  Discard unmarked objects, compact down marked
     * objects and fix up the atom pool.
     *
     * Deleteing an atom from the atom pool is (say) once as expensive
     * as adding one.  Use this to determine which is quicker; rebuilding
     * the atom pool or deleting dead ones.
     */
    if (ndead_atoms > (ici_natoms - ndead_atoms))
    {
        /*
         * Rebuilding the atom pool is a better idea. Zap the dead
         * atoms in the atom pool (thus breaking it) then rebuild
         * it (which doesn't care it isn't a hash table any more).
         */
        a = &atoms[atomsz];
        while (--a >= atoms)
        {
            if ((o = *a) != NULL && o->o_nrefs == 0 && !o->marked())
                *a = NULL;
        }
        ici_natoms -= ndead_atoms;
        /*
         * Call grow_atoms() to reuild the atom pool. On entry it isn't
         * a legal hash table, but grow_atoms() doesn't care. We make the
         * new one the same size as the old one. Should we?
         */
        if (ici_natoms * 4 > atomsz)
            atomsz *= 4;
        grow_atoms_core(atomsz);

        for (a = b = objs; a < objs_top; ++a)
        {
            o = *a;
            if (!o->marked())
            {
                ici_freeo(o);
            }
            else
            {
                o->clrmark();
                *b++ = o;
            }
        }
        objs_top = b;
    }
    else
#endif
    {
        /*
         * Faster to delete dead atoms as we go.
         */
        for (a = b = objs; a < objs_top; ++a)
        {
            o = *a;
            if (!o->marked())
            {
                if (!o->isatom() || unatom(o) == 0)
                {
                    ici_freeo(o);
                }
            }
            else
            {
                o->clrmark();
                *b++ = o;
            }
        }
        objs_top = b;
    }
/*
printf("mem=%ld vs. %ld, nobjects=%d, ici_natoms=%d\n", mem, ici_mem, objs_top - objs, ici_natoms);
*/
    /*
     * Set ici_mem_limit (which is the point at which to trigger a
     * new call to us) to 1.5 times what is currently allocated, but
     * with a special cases for small sizes.
     */
#   if ALLCOLLECT
    ici_mem_limit = 0;
#   else
    if (ici_mem < 128 * 1024)
    {
	ici_mem_limit = 256 * 1024;
    }
    else
    {
	ici_mem_limit = (ici_mem * 3) / 2;
    }
#   endif
    --supress_collect;
}

#ifndef NDEBUG
void
ici_dump_refs()
{
    object           **a;
    char                n[30];
    int                 spoken;

    spoken = 0;
    for (a = objs; a < objs_top; ++a)
    {
        if ((*a)->o_nrefs == 0)
        {
            continue;
        }
        if (!spoken)
        {
            printf("The following ojects have spurious left-over reference counts...\n");
            spoken = 1;
        }
        printf("%d 0x%08lX: %s\n", (*a)->o_nrefs, (unsigned long)*a, objname(n, *a));
    }

}
#endif

/*
 * Garbage collection triggered by other than our internal mechanmism.
 * Don't do this unless you really must. It will free all memory it can
 * but will reduce subsequent performance.
 */
void reclaim()
{
    collect();
}


#ifndef NDEBUG

object   *traceobj;

void object::incref(object *o)
{
    if (o == traceobj)
    {
        printf("incref traceobj(%d)\n", o->o_tcode);
    }
    if ((unsigned char)o->o_nrefs == (unsigned char)0x7F)
    {
        printf("Oops: ref count overflow\n");
        abort();
    }
    if (++o->o_nrefs > 50)
    {
        printf("Warning: nrefs %d > 10\n", o->o_nrefs);
        fflush(stdout);
    }
}

void object::decref(object *o)
{
    if (o == traceobj)
    {
        printf("decref traceobj(%d)\n", o->o_tcode);
    }
    if (--o->o_nrefs < 0)
    {
        printf("Oops: ref count underflow\n");
        abort();
    }
}

void
bughunt_rego(object *o)
{
    if (o == traceobj)
    {
        printf("rego traceobj(%d)\n", o->o_tcode);
    }
    o->o_leafz = 0;                     
    if (objs_top < objs_limit)
        *objs_top++ = o;
    else
        grow_objs(o);
}
#endif

} // namespace ici

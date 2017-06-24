#define ICI_CORE
#include "fwd.h"

#if defined ICI_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

namespace ici
{

/*
 * The amount of memory we currently have allocated, and a limit.
 * When we reach the limit, a garbage collection is triggered (which
 * will presumably reduce ici_mem and re-evaluate the limit).
 */
size_t                  ici_mem;
size_t                  ici_mem_limit;

/*
 * A count and estimated total size of outstanding allocs done through
 * ici_alloc and yet freed with ici_free.
 */
size_t                  ici_n_allocs;
size_t                  ici_alloc_mem;

#if !ICI_ALLALLOC

/*
 * A chunk of memory in which to keep dense allocations of small
 * objects to avoid boundary word overheads and allow simple fast
 * free lists.
 */
typedef struct achunk   achunk_t;
struct achunk
{
    char                c_data[4088];
    achunk_t            *c_next;
};

/*
 * The base pointers of our four fast free lists.
 */
char                    *ici_flists[4];

/*
 * The current next available block, and limits, within each of
 * the allocation chunks for each of the size categories we have
 * for small objects.
 */
static char             *mem_next[4];
static char             *mem_limit[4];

/*
 * The global list of all chunks of small dense objects we have allocated.
 * We just keep this so we can free them all on interpreter shutdown.
 */
static achunk_t         *ici_achunks;

#endif /* ICI_ALLALLOC */


/*
 * Allocate an object of the given 'size'.  Return NULL on failure, usual
 * conventions.  The resulting object must be freed with ici_nfree() and only
 * ici_nfree().  Note that ici_nfree() also requires to know the size of the
 * object being freed.
 *
 * This function is preferable to ici_alloc(). It should be used if
 * you can know the size of the allocation when the free happens so
 * you can call ici_nfree(). If this isn't the case you will have to
 * use ici_alloc().
 *
 * See also: 'ICIs allocation functions', ici_talloc(), ici_alloc(),
 * ici_nfree().
 *
 * This --func-- forms part of the --ici-api--.
 */
void *
ici_nalloc(size_t z)
{
    char                *r;
#if !ICI_ALLALLOC
    int                 fi;
    static char const   which_flist[] = {0, 1, 2, 2, 3, 3, 3, 3};
#endif

    if ((ici_mem += z) > ici_mem_limit)
    {
        collect();
    }

#if !ICI_ALLALLOC

    if (z <= 64)
    {
        fi = which_flist[(z - 1) >> 3];
    }
    else
    {
        fi = z;
    }
    if (fi < (int)nels(ici_flists))
    {
        char                **fp;
        int                 cz;
        achunk_t            *c;

        /*
         * Small block. Try to get it off one of the fast free lists.
         */
        fp = &ici_flists[fi];
        if ((r = *fp) != NULL)
        {
            *fp = *(char **)r;
            return r;
        }
        /*
         * Free list empty. Rip off a bit more memory from the current
         * chunk.
         */
        cz = 8 << fi;
        if (mem_next[fi] + cz <= mem_limit[fi])
        {
            r = mem_next[fi];
            mem_next[fi] += cz;
            return r;
        }
        /*
         * Current chunk empty. Allocate another one.
         */
        if ((c = (achunk_t *)malloc(sizeof (achunk_t))) == NULL)
        {
            collect();
            if ((c = (achunk_t *)malloc(sizeof (achunk_t))) == NULL)
                goto fail;
        }
        c->c_next = ici_achunks;
        ici_achunks = c;
        mem_next[fi] = (char *)(((unsigned long)c->c_data + 0x3F) & ~0x3F);
        mem_limit[fi] = &c->c_data[nels(c->c_data)];
        r = mem_next[fi];
        mem_next[fi] += cz;
        return r;
    }
#endif /* ICI_ALLALLOC */

    if ((r = (char *)malloc(z)) == NULL)
    {
        collect();
        if ((r = (char *)malloc(z)) == NULL)
            goto fail;
    }
    return r;

fail:
    set_error("ran out of memory");
    return NULL;
}

/*
 * Free an object allocated with ici_nalloc(). The 'size' passed here
 * must be exactly the same size passed to ici_nalloc() when the
 * allocation was made. If you don't know the size, you should have
 * called ici_alloc() in the first place.
 *
 * See also: 'ICIs allocation functions', ici_nalloc().
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_nfree(void *p, size_t z)
{
#if !ICI_ALLALLOC
    int                 fi;
    static char const   which_flist[] = {0, 1, 2, 2, 3, 3, 3, 3};
#endif

    ici_mem -= z;
#if !ICI_ALLALLOC
    if (z <= 64)
    {
        /*
         * Small block. Just push it onto one of our fast free lists.
         */
        fi = which_flist[(z - 1) >> 3];
        *(char **)p = ici_flists[fi];
        ici_flists[fi] = (char *)p;
    }
    else
#endif
    {
        free(p);
    }
}

/*
 * Allocate a block of size 'z'. This just maps to a raw malloc() but
 * does garbage collection as necessary and attempts to track memory
 * usage to control when the garbage collector runs. Blocks allocated
 * with this must be freed with ici_free().
 *
 * It is preferable to use ici_talloc(), or failing that, ici_nalloc(),
 * instead of this function. But both require that you can match the
 * allocation by calling ici_tfree() or ici_nalloc() with the original
 * type/size you passed in the allocation call. Those functions use
 * dense fast free lists for small objects, and track memory usage
 * better.
 *
 * See also: 'ICIs allocation functions', ici_free(), ici_talloc(),
 * ici_nalloc().
 *
 * This --func-- forms part of the --ici-api--.
 */
void *
ici_alloc(size_t z)
{
    void                *p;

#if ALLCOLLECT
    collect();
#else
    if ((ici_mem += z) > ici_mem_limit)
    {
        collect();
    }
#endif

    ++ici_n_allocs;
    ici_alloc_mem += z;
    if ((p = malloc(z)) == NULL)
    {
        collect();
        if ((p = malloc(z)) == NULL)
        {
            set_error("ran out of memory");
            return NULL;
        }
    }
    return p;
}

/*
 * Free a block allocated with ici_alloc().
 *
 * See also: 'ICIs allocation functions', ici_alloc(), ici_tfree(),
 * ici_nfree().
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_free(void *p)
{
    ptrdiff_t           z;

    /*
     * This makes a really dodgy attempt to track memory usage. Because
     * we don't know how big the block being freed is, we just assume it
     * is 1/Nth the size of the number of such mysterious allocations
     * we have. Does a division - yuk.
     */
    z = ici_alloc_mem / ici_n_allocs;
    if (z > ptrdiff_t(ici_mem))
    {
        z = ici_mem;
    }
    if (z > ptrdiff_t(ici_alloc_mem))
    {
        z = ici_alloc_mem;
    }
    ici_mem -= z;
    ici_alloc_mem -= z;
    --ici_n_allocs;
    free(p);
}

/*
 * Drop (free) all memory associated with our dense allocations of small
 * objects and fast free lists. For use in interpreter shutdown. Will
 * cause total disaster if called at any other time.
 */
void drop_all_small_allocations()
{
#if !ICI_ALLALLOC
    achunk_t            *c;

    while ((c = ici_achunks) != NULL)
    {
        ici_achunks = c->c_next;
        free(c);
    }
#endif /* ICI_ALLALLOC */
}

} // namespace ici

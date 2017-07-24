// -*- mode:c++ -*-

#ifndef ICI_ALLOC_H
#define ICI_ALLOC_H

#if defined ICI_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

namespace ici
{

/*
 * Define this to 1 to prevent the use of fast free lists.  All allocations will go
 * to the native malloc.  Can be very useful to set, along with ALLCOLLECT in
 * object.c, during debug and test.
 *
 * It's possible some systems have a malloc so efficient that the overhead
 * code for the dense object allocations will make this is a net penalty.  So
 * we allow the config file to set it ahead of this definition.
 */
#if     !defined(ICI_ALLALLOC)
#define ICI_ALLALLOC    0   /* Always call malloc, no caches. */
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

extern char             *ici_flists[4];
extern size_t           ici_mem;
extern size_t           ici_mem_limit;
extern void             *ici_nalloc(size_t);
extern void             ici_nfree(void *, size_t);
extern void             *ici_alloc(size_t);
extern void             ici_free(void *);

/*
 * Allocate an object of the given type 't'. Return NULL on failure, usual
 * conventions. The resulting object *must* be freed with 'ici_tfree()'.
 * Note that 'ici_tfree()' also requires to know the type of the object
 * being freed.
 *
 * This --func-- forms part of the --ici-api--.
 */
template <typename T> T *ici_talloc() { return (T *)ici_nalloc(sizeof (T)); }

/*
 * Free the object 'o' which was allocated by a call to 'ici_talloc()' with
 * the type 't'.  The object *must* have been allocated with 'ici_talloc()'.
 *
 * This --func-- forms part of the --ici-api--.
 */
template <typename T> void ici_tfree(void *p) { ici_nfree(p, sizeof (T)); }

/*
 * End of ici.h export. --ici.h-end--
 */

#if     !ICI_ALLALLOC
/*
 * In the core, ici_talloc and ici_tfree are done semi in-line, (unless
 * ICI_ALLALLOC is set).  This way they often result in no function calls.
 * They are *not* done this way in the extension API because it would make the
 * binary interface too fragile with respect changes in the internals.
 */

/*
 * Determine what free list an object of the given type is appropriate
 * for, or the size of the object if it is too big. We assume the
 * compiler will reduce this constant expression to a constant at
 * compile time.
 */
template <typename T> inline size_t ICI_FLIST() {
    return sizeof (T) <=  8 ? 0
        : sizeof (T) <= 16 ? 1
        : sizeof (T) <= 32 ? 2
        : sizeof (T) <= 64 ? 3
        : sizeof (T);
}

/*
 * Is an object of this type of a size suitable for one of the
 * fast free lists?
 */
inline bool ICI_FLOK(size_t n) { return n <= 64; }
template <typename T> inline bool ICI_TFLOK() { return ICI_FLOK(sizeof (T)); }

inline void *ici_talloc_n(char *p, size_t index, size_t n)
{
    ici_flists[index] = *(char **)p;
    ici_mem += n;
    return p;
}

/*
 * If the object is too big for a fast free list, these functions should reduce
 * to a simple function call.  If it is small, it will reduce to an attempt to
 * pop a block off the correct fast free list, but call the function if the
 * list is empty.
 */
template <typename T>
inline T *ici_talloc_core()
{
    if (ICI_TFLOK<T>()) {
        if (char *fl = ici_flists[ICI_FLIST<T>()]) {
            return (T *)ici_talloc_n(fl, ICI_FLIST<T>(), sizeof (T));
        }
    }
    return (T *)ici_nalloc(sizeof (T));
}

/* tfree */

inline void ici_tfree_n(void *p, size_t list, size_t n)
{
    *(char **)(p) = ici_flists[list];
    ici_flists[list] = (char *)(p);
    ici_mem -= n;
}

template <typename T> inline void ici_tfree_core(void *p)
{
    if (ICI_TFLOK<T>())
        ici_tfree_n(p, ICI_FLIST<T>(), sizeof (T));
    else
        ici_nfree(p, sizeof (T));
}

#define ici_talloc(t)  ici_talloc_core<t>()
#define ici_tfree(p,t) ici_tfree_core<t>(p)

#endif  /* ICI_ALLALLOC */

} // namespace ici

#endif /* ICI_ALLOC_H */

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
extern char             *ici_fltmp;

extern long             ici_mem;
extern long             ici_mem_limit;

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
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_talloc(t)   ((t *)(ici_nalloc(sizeof(t))))

/*
 * Free the object 'o' which was allocated by a call to 'ici_talloc()' with
 * the type 't'.  The object *must* have been allocated with 'ici_talloc()'.
 *
 * This --macro-- forms part of the --ici-api--.
 */

#define ici_tfree(p, t) ici_nfree((p), sizeof(t))

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
#define ICI_FLIST(t)    ( sizeof(t) <=  8 ? 0 \
                        : sizeof(t) <= 16 ? 1 \
                        : sizeof(t) <= 32 ? 2 \
                        : sizeof(t) <= 64 ? 3 \
                        : sizeof(t)	      \
			)

/*
 * Is an object of this type of a size suitable for one of the
 * fast free lists?
 */
#define ICI_FLOK(n)     ((n) <= 64)
#define ICI_TFLOK(t)	ICI_FLOK(sizeof(t))

static inline void *ici_talloc_n(char *p, size_t index, size_t n)
{
    ici_flists[index] = *(char **)p;
    ici_mem += n;
    return p;
}

/*
 * If the object is too big for a fast free list, these macros should reduce
 * to a simple function call.  If it is small, it will reduce to an attempt to
 * pop a block off the correct fast free list, but call the function if the
 * list is empty.
 */
#   undef  ici_talloc
#   define ici_talloc(t)					    	\
    (								    	\
        ICI_TFLOK(t) && (ici_fltmp = ici_flists[ICI_FLIST(t)]) != NULL	\
        ?								\
        (t *)ici_talloc_n(ici_fltmp, ICI_FLIST(t), sizeof(t))           \
        :								\
        (t *)ici_nalloc(sizeof(t))                                      \
    )

/* tfree */

#   undef ici_tfree
static inline void ici_tfree_n(void *p, size_t list, size_t n)
{
    *(char **)(p) = ici_flists[list];
    ici_flists[list] = (char *)(p);
    ici_mem -= n;
}

#   define ici_tfree(p, t)				\
    do							\
    {							\
	if (ICI_TFLOK(t))				\
	{						\
	    ici_tfree_n(p, ICI_FLIST(t), sizeof(t));	\
	}					 	\
        else						\
	{						\
	    ici_nfree((p), sizeof(t));			\
	}					 	\
    }							\
    while (0)

#endif  /* ICI_ALLALLOC */

} // namespace ici

#endif /* ICI_ALLOC_H */

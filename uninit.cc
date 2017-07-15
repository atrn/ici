#define ICI_CORE
#include "buf.h"
#include "exec.h"
#include "str.h"
#include "re.h"
#include "archiver.h"
#include "wrap.h"

namespace ici
{

static wrap *wraps;

/*
 * Register the function 'func' to be called at ICI interpreter shutdown
 * (i.e. 'uninit()' call).
 *
 * The caller must supply a 'wrap' struct, which is usually statically
 * allocated. This structure will be linked onto an internal list and
 * be unavailable till after 'uninit()' is called.
 *
 * This --func-- forms part of the --ici-api--.
 */
void atexit(void (*func)(), wrap *w)
{
    w->w_next = wraps;
    w->w_func = func;
    wraps = w;
}

/*
 * Shut down the interpreter and clean up any allocations.  This function is
 * the reverse of 'init()'.  It's first action is to call any wrap-up
 * functions registered through 'atexit()'
 *
 * Calling 'ici_init()' again after calling this hasn't been adequately
 * tested.
 *
 * This routine currently does not handle shutdown of other threads,
 * either gracefully or ungracefully. They are all left blocked on the
 * global ICI mutex without any help of recovery.
 *
 * This --func-- forms part of the --ici-api--.
 */
void uninit()
{
    int           i;
    exec    *x;
    extern str    *ver_cache;
    extern regexp *smash_default_re;

    /*
     * This catches the case where uninit() is called without ici_init
     * ever being called.
     */
    assert(o_zero != NULL);
    if (o_zero == NULL)
        return;

    /*
     * Clean up anything registered by modules that are only optionally
     * compiled in, or loaded modules that register wrap-up functions.
     */
    while (wraps != NULL)
    {
        (*wraps->w_func)();
        wraps = wraps->w_next;
    }

    archive_uninit();

    /*
     * Clean up ICI variables used by various bits of ICI.
     */
    for (i = 0; i < (int)nels(small_ints); ++i)
    {
        small_ints[i]->decref();
        small_ints[i] = NULL;
    }
    if (ver_cache != NULL)
        ver_cache->decref();
    if (smash_default_re != NULL)
        smash_default_re->decref();

    /* Call uninitialisation functions for compulsory bits of ICI. */
    uninit_compile();
    uninit_cfunc();

    /*
     * Do a GC to free things that might require reference to the
     * exec state before we discard it.
     */
    reclaim();

    /*
     * Active threads, including the main one, will count reference counts
     * for their exec structs. But finished ones will have none. We don't
     * really care. Just zap them all and let the garbage collector sort
     * them out. This routine doesn't really handle shutdown with outstanding
     * threads running (not that they can actually be running -- we have the
     * mutex).
     */
    for (x = execs; x != NULL; x = x->x_next)
        x->o_nrefs = 0;

    /*
     * We don't decref the static cached copies of our stacks, because if we
     * did the garbage collector would try to free them (they are static
     * objects, so that would be bad).  However we do empty the stacks.
     */
    vs.a_top = vs.a_base;
    os.a_top = os.a_base;
    xs.a_top = xs.a_base;

    /*
     * OK, so do one final garbage collect to free all this stuff that should
     * now be unreferenced.
     */
    reclaim();

    /*
     * Now free the allocated part of our three special static stacks.
     */
    ici_nfree(vs.a_base, (vs.a_limit - vs.a_base) * sizeof (object *));
    ici_nfree(os.a_base, (os.a_limit - os.a_base) * sizeof (object *));
    ici_nfree(xs.a_base, (xs.a_limit - xs.a_base) * sizeof (object *));

#if 1 && !defined(NDEBUG)
    vs.decref();
    os.decref();
    xs.decref();
    {
        extern void ici_dump_refs();

        ici_dump_refs();
    }
#endif

    if (buf != error)
    {
        /* Free the general purpose buffer. ### Hmm... If we do this we can't
        return error from ici_main.*/
        ici_nfree(buf, bufz + 1);
    }
    buf = NULL;
    bufz = 0;

    /*
     * Destroy the now empty atom pool and list of registered objects.
     */
    ici_nfree(atoms, atomsz * sizeof (object *));
    atoms = NULL;
    ici_nfree(objs, (objs_limit - objs) * sizeof (object *));
    objs = NULL;

    drop_all_small_allocations();
    /*fprintf(stderr, "ici_mem = %ld, n = %d\n", ici_mem, ici_n_allocs);*/

#if 1 && defined(_WIN32) && !defined(NDEBUG)
    _CrtDumpMemoryLeaks();
#endif
/*    {
        extern long attempt, miss;
        printf("%d/%d %f\n", miss, attempt, (float)miss/attempt);
    }*/
}

} // namespace ici

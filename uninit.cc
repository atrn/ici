#define ICI_CORE
#include "buf.h"
#include "exec.h"
#include "str.h"
#include "re.h"
#include "archive.h"
#include "wrap.h"

namespace ici
{

static ici_wrap_t       *wraps;

/*
 * Register the function 'func' to be called at ICI interpreter shutdown
 * (i.e. 'ici_uninit()' call).
 *
 * The caller must supply a 'ici_wrap_t' struct, which is usually statically
 * allocated. This structure will be linked onto an internal list and
 * be unavailable till after 'ici_uninit()' is called.
 *
 * This --func-- forms part of the --ici-api--.
 */
void
ici_atexit(void (*func)(), ici_wrap_t *w)
{
    w->w_next = wraps;
    w->w_func = func;
    wraps = w;
}

/*
 * Shut down the interpreter and clean up any allocations.  This function is
 * the reverse of 'ici_init()'.  It's first action is to call any wrap-up
 * functions registered through 'ici_atexit()'
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
void
ici_uninit()
{
    int                 i;
    ici_exec_t          *x;
    extern ici_str_t    *ici_ver_cache;
    extern ici_regexp_t *ici_smash_default_re;

    /*
     * This catches the case where ici_uninit() is called without ici_init
     * ever being called.
     */
    assert(ici_zero != NULL);
    if (ici_zero == NULL)
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

    ici_archive_uninit();

    /*
     * Clean up ICI variables used by various bits of ICI.
     */
    for (i = 0; i < (int)nels(ici_small_ints); ++i)
    {
        ici_small_ints[i]->decref();
        ici_small_ints[i] = NULL;
    }
    if (ici_ver_cache != NULL)
        ici_ver_cache->decref();
    if (ici_smash_default_re != NULL)
        ici_smash_default_re->decref();

    /* Call uninitialisation functions for compulsory bits of ICI. */
    ici_uninit_compile();
    ici_uninit_cfunc();

    /*
     * Do a GC to free things that might require reference to the
     * exec state before we discard it.
     */
    ici_reclaim();

    /*
     * Active threads, including the main one, will count reference counts
     * for their exec structs. But finished ones will have none. We don't
     * really care. Just zap them all and let the garbage collector sort
     * them out. This routine doesn't really handle shutdown with outstanding
     * threads running (not that they can actually be running -- we have the
     * mutex).
     */
    for (x = ici_execs; x != NULL; x = x->x_next)
        x->o_nrefs = 0;

    /*
     * We don't decref the static cached copies of our stacks, because if we
     * did the garbage collector would try to free them (they are static
     * objects, so that would be bad).  However we do empty the stacks.
     */
    ici_vs.a_top = ici_vs.a_base;
    ici_os.a_top = ici_os.a_base;
    ici_xs.a_top = ici_xs.a_base;

    /*
     * OK, so do one final garbage collect to free all this stuff that should
     * now be unreferenced.
     */
    ici_reclaim();

    /*
     * Now free the allocated part of our three special static stacks.
     */
    ici_nfree(ici_vs.a_base, (ici_vs.a_limit - ici_vs.a_base) * sizeof(ici_obj_t *));
    ici_nfree(ici_os.a_base, (ici_os.a_limit - ici_os.a_base) * sizeof(ici_obj_t *));
    ici_nfree(ici_xs.a_base, (ici_xs.a_limit - ici_xs.a_base) * sizeof(ici_obj_t *));

#if 1 && !defined(NDEBUG)
    ici_vs.decref();
    ici_os.decref();
    ici_xs.decref();
    {
        extern void ici_dump_refs();

        ici_dump_refs();
    }
#endif

    if (ici_buf != ici_error)
    {
        /* Free the general purpose buffer. ### Hmm... If we do this we can't
        return ici_error from ici_main.*/
        ici_nfree(ici_buf, ici_bufz + 1);
    }
    ici_buf = NULL;
    ici_bufz = 0;

    /*
     * Destroy the now empty atom pool and list of registered objects.
     */
    ici_nfree(ici_atoms, ici_atomsz * sizeof (ici_obj_t *));
    ici_atoms = NULL;
    ici_nfree(ici_objs, (ici_objs_limit - ici_objs) * sizeof (ici_obj_t *));
    ici_objs = NULL;

    ici_drop_all_small_allocations();
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

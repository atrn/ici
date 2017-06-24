#define ICI_CORE
/*
 * Coarse ICI Profiler
 * -------------------
 *
 * This profiler measures actual ellapsed time so it's only very useful for
 * quite coarse profiling tasks.
 *
 * Possible improvements:
 *
 * - Take into account that clock() may not actually return ms, should use
 *   CLOCKS_PER_SEC constant.
 *
 * - Measure and subtract garbage collection time.
 *
 * - Raise this thread's priority to ensure that other threads amd processes
 *   don't make it appear that more time has been spent than actually has.
 *
 *
 * Win32
 * -----
 * On Win32 we now use the Windows multimedia timer for single millisecond
 * resolution.  This requires that you
 * link with winmm.lib.  If you don't want to do this you can always use
 * clock() on Win32 too, but that has a lower resolution.
 */
#ifndef NOPROFILE

#include "fwd.h"
#include "profile.h"
#include "str.h"
#include "func.h"
#include "op.h"
#include "exec.h"
#include "null.h"
#include "struct.h"
#include <time.h>

/* This is required for the high resolution timer. */
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace ici
{

/*
 * Set to true by an ICI call to 'profile()'
 */
int ici_profile_active = 0;


/*
 * The call currently being executed.
 */
ici_profilecall_t *ici_prof_cur_call = NULL;


/*
 * The function to call when profiling completes.  This might (for example)
 * display the call graph in a dialog box.
 */
void (*ici_prof_done_callback)(ici_profilecall_t *) = NULL;


/*
 * The file to write profile results to when complete.
 *
 * This file contains ICI code that when parsed will build a data structure:
 *
 *  auto profile = [struct
 *                     total = <time in ms for this call>,
 *                     calls = [set <nested profile structs...>],
 *                 ];
 */
char ici_prof_outfile[512] = "";


/*
 * time_in_ms
 *
 * Operating system-specific mechanism for retrieving the current time in
 * milliseconds.
 *
 * Returns:
 *  A value in ms relative to some point in time that's fixed for the
 *  duration of the program.
 */
#ifdef _WIN32
#define time_in_ms() timeGetTime()
#else
#define time_in_ms() ((long)(clock()))
#endif

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t profilecall_type::mark(ici_obj_t *o)
{
    o->setmark();
    auto pf = ici_profilecallof(o);
    return typesize() + ici_mark(pf->pc_calls) + (pf->pc_calledby == NULL ? 0 : ici_mark(pf->pc_calledby));
}

/*
 * Parameters:
 *  called_by   The ici_profilecall_t for the function calling this 'f'.
 *
 * Returns:
 *  A new ici_profilecall_t object.
 */
ici_profilecall_t *
ici_profilecall_new(ici_profilecall_t *called_by)
{
    ici_profilecall_t *pc;

    /*
     * We always explicitly create these buggers, they aren't atomic.
     *
     *    if ((g = atom_gob(gob)) != NULL)
     *    {
     *        g->incref();
     *        return g;
     *    }
     */

    /* Allocate storage for it. */
    if ((pc = ici_talloc(ici_profilecall_t)) == NULL)
        return NULL;

    /* Fill in the bits common to all ICI objects. */
    ICI_OBJ_SET_TFNZ(pc, ICI_TC_PROFILECALL, 0, 1, 0);

    /* Fill in ici_profilecall specific bits. */
    pc->pc_calledby = called_by;
    pc->pc_calls = ici_struct_new();
    if (pc->pc_calls == NULL)
    {
        ici_tfree(pc, ici_profilecall_t);
        return NULL;
    }
    pc->pc_calls->decref();
    pc->pc_total = 0;
    pc->pc_laststart = 0;
    pc->pc_call_count = 0;

    /* Link it in to the global list of objects. */
    ici_rego(pc);

    return pc;
}


/*
 * Parameters:
 *  done        A function to call when profiling completes.  Its only
 *              parameter is the root of the profiling call graph.
 */
void
ici_profile_set_done_callback(void (*done)(ici_profilecall_t *))
{
    ici_prof_done_callback = done;
}


/*
 * profile
 *
 *  Called to begin profiling.  Profiling ends when the function this call
 *  was made from returns.
 *
 * Parameters:
 *  outfile     The path to a file into which to dump the results of the
 *              profiling session.  See the comment with ici_prof_outfile for
 *              the format of the file.  If this parameter is omitted then
 *              the profiling is not written to any file.  However, see
 *              ici_profile_set_done_callback() for another mechanism to
 *              get at the data.
 */
static int
f_profile(...)
{
    char *outfile;
    assert(!ici_profile_active);

    /* Check parameters. */
    if (NARGS() > 1)
        return ici_argcount(1);
    if (NARGS() == 1)
    {
        /* Check and store the path that profiling info will be saved to. */
        if (typecheck("s", &outfile))
            return 1;
        strcpy(ici_prof_outfile, outfile);
    }

    /* Start profiling. */
    ici_profile_active = 1;
    return ici_null_ret();
}


/*
 *  This is called whenever ICI calls a function.
 */
void
ici_profile_call(ici_func_t *f)
{
    ici_profilecall_t *pc;
    time_t start;
    start = time_in_ms();

    /* Has this function been called from the current function before? */
    assert(ici_prof_cur_call != NULL);
    if (isnull(pc = ici_profilecallof(ici_fetch(ici_prof_cur_call->pc_calls, f))))
    {
        /* No, create a new record. */
        pc = ici_profilecall_new(ici_prof_cur_call);
        assert(pc != NULL);
        pc->decref();

        /* Add it to the calling function. */
        ici_assign(ici_prof_cur_call->pc_calls, f, pc);
    }

    /* Switch context to the new function and remember when we entered it. */
    pc->pc_laststart = start;
    ++ pc->pc_call_count;
    ici_prof_cur_call = pc;
}


/*
 *  Dumps a ici_profilecall_t to a file.  Warning: this is recursive.
 *
 * Parameters:
 *  of      Output file.
 *  pc      The call to write out.
 *  indent  Number of spaces to indent with.
 */
static void
write_outfile(FILE *of, ici_profilecall_t *pc, int indent)
{
    ici_sslot_t*sl;
    char    *p;

    fputs("[struct\n", of);
    fprintf(of, "%*stotal = %ld,\n", indent + 1, "", pc->pc_total);
    fprintf(of, "%*scall_count = %ld,\n", indent + 1, "", pc->pc_call_count);
    fprintf(of, "%*scalls = [struct\n", indent + 1, "");
    for
    (
        sl = pc->pc_calls->s_slots + pc->pc_calls->s_nslots - 1;
        sl >= pc->pc_calls->s_slots;
        -- sl
    )
    {
        if (sl->sl_key != NULL)
        {
            char    n1[ICI_OBJNAMEZ];

            ici_objname(n1, sl->sl_key);
            fprintf(of, "%*s(\"", indent + 2, "");
            for (p = n1; *p != '\0'; ++p)
            {
                if (*p != '\"' && *p != '\\')
                {
                    fputc(*p, of);
                }
            }
            fprintf(of, "\") = ");
            write_outfile(of, ici_profilecallof(sl->sl_value), indent + 2);
            fputs(",\n", of);
        }
    }
    fprintf(of, "%*s],\n", indent + 1, "");
    fprintf(of, "%*s]", indent, "");
}


/*
 *  Called whenever ICI returns from a function.
 */
void
ici_profile_return()
{
    /* Is this the return immediately after the call to profile()? */
    if (ici_prof_cur_call == NULL)
    {
        /* Yes, create the top-level ici_profilecall object. */
        ici_prof_cur_call = ici_profilecall_new(NULL);
        ici_prof_cur_call->pc_laststart = time_in_ms();
        #ifdef _WIN32
            timeBeginPeriod(1);
        #endif
    }
    else
    {
        /* Add the time taken by this call. */
        ici_prof_cur_call->pc_total += time_in_ms() - ici_prof_cur_call->pc_laststart;

        /* Have we run out? */
        if (ici_prof_cur_call->pc_calledby == NULL)
        {
            /* Yeah we've run out, end of profile. */
            #ifdef _WIN32
                timeEndPeriod(1);
            #endif

            if (ici_prof_done_callback != NULL)
            {
                /* A callback has been supplied, this will know how to
                 * display or save the data. */
                ici_prof_done_callback(ici_prof_cur_call);
            }

            if (strlen(ici_prof_outfile) > 0)
            {
                /* A path has been supplied to save the data. */
                FILE *of = fopen(ici_prof_outfile, "w");
                if (of != NULL)
                {
                    fputs("auto profile = ", of);
                    write_outfile(of, ici_prof_cur_call, 0);
                    fputs(";\n", of);
                    fclose(of);
                }
            }

            /* No more profiling. */
            ici_prof_cur_call->decref();
            ici_prof_cur_call = NULL;
            ici_profile_active = 0;
        }
        else
        {
            /* No, still going strong, pop up our tree of calls. */
            ici_prof_cur_call = ici_prof_cur_call->pc_calledby;
        }
    }
}


/*
 * ICI functions exported for profiling.
 */
ICI_DEFINE_CFUNCS(profile)
{
    ICI_DEFINE_CFUNC(profile, f_profile),
    ICI_CFUNCS_END()
};

#endif

} // namespace ici

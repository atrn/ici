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
#include "cfunc.h"
#include "op.h"
#include "exec.h"
#include "null.h"
#include "map.h"
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
int profile_active = 0;


/*
 * The call currently being executed.
 */
profilecall *ici_prof_cur_call = nullptr;


/*
 * The function to call when profiling completes.  This might (for example)
 * display the call graph in a dialog box.
 */
void (*ici_prof_done_callback)(profilecall *) = nullptr;


/*
 * The file to write profile results to when complete.
 *
 * This file contains ICI code that when parsed will build a data structure:
 *
 *  auto profile = [map
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
size_t profilecall_type::mark(object *o)
{
    auto pf = profilecallof(o);
    return type::mark(pf) + ici_mark(pf->pc_calls) + mark_optional(pf->pc_calledby);
}

/*
 * Parameters:
 *  called_by   The profilecall for the function calling this 'f'.
 *
 * Returns:
 *  A new profilecall object.
 */
profilecall *
profilecall_new(profilecall *called_by)
{
    profilecall *pc;

    /*
     * We always explicitly create these buggers, they aren't atomic.
     *
     *    if ((g = atom_gob(gob)) != nullptr)
     *    {
     *        incref(g);
     *        return g;
     *    }
     */

    /* Allocate storage for it. */
    if ((pc = ici_talloc(profilecall)) == nullptr)
        return nullptr;

    /* Fill in the bits common to all ICI objects. */
    set_tfnz(pc, TC_PROFILECALL, 0, 1, 0);

    /* Fill in profilecall specific bits. */
    pc->pc_calledby = called_by;
    pc->pc_calls = new_map();
    if (pc->pc_calls == nullptr)
    {
        ici_tfree(pc, profilecall);
        return nullptr;
    }
    decref(pc->pc_calls);
    pc->pc_total = 0;
    pc->pc_laststart = 0;
    pc->pc_call_count = 0;

    /* Link it in to the global list of objects. */
    rego(pc);

    return pc;
}


/*
 * Parameters:
 *  done        A function to call when profiling completes.  Its only
 *              parameter is the root of the profiling call graph.
 */
void
profile_set_done_callback(void (*done)(profilecall *))
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
 *              profile_set_done_callback() for another mechanism to
 *              get at the data.
 */
static int
f_profile(...)
{
    char *outfile;
    assert(!profile_active);

    /* Check parameters. */
    if (NARGS() > 1)
        return argcount(1);
    if (NARGS() == 1)
    {
        /* Check and store the path that profiling info will be saved to. */
        if (typecheck("s", &outfile))
            return 1;
        strcpy(ici_prof_outfile, outfile);
    }

    /* Start profiling. */
    profile_active = 1;
    return null_ret();
}


/*
 *  This is called whenever ICI calls a function.
 */
void profile_call(func *f)
{
    profilecall *pc;
    time_t start;
    start = time_in_ms();

    /* Has this function been called from the current function before? */
    assert(ici_prof_cur_call != nullptr);
    if (isnull(pc = profilecallof(ici_fetch(ici_prof_cur_call->pc_calls, f))))
    {
        /* No, create a new record. */
        pc = profilecall_new(ici_prof_cur_call);
        assert(pc != nullptr);
        decref(pc);

        /* Add it to the calling function. */
        ici_assign(ici_prof_cur_call->pc_calls, f, pc);
    }

    /* Switch context to the new function and remember when we entered it. */
    pc->pc_laststart = start;
    ++ pc->pc_call_count;
    ici_prof_cur_call = pc;
}


/*
 *  Dumps a profilecall to a file.  Warning: this is recursive.
 *
 * Parameters:
 *  of      Output file.
 *  pc      The call to write out.
 *  indent  Number of spaces to indent with.
 */
static void write_outfile(FILE *of, profilecall *pc, int indent)
{
    slot *sl;
    char  *p;

    fputs("[map\n", of);
    fprintf(of, "%*stotal = %ld,\n", indent + 1, "", pc->pc_total);
    fprintf(of, "%*scall_count = %ld,\n", indent + 1, "", pc->pc_call_count);
    fprintf(of, "%*scalls = [map\n", indent + 1, "");
    for
    (
        sl = pc->pc_calls->s_slots + pc->pc_calls->s_nslots - 1;
        sl >= pc->pc_calls->s_slots;
        -- sl
    )
    {
        if (sl->sl_key != nullptr)
        {
            char    n1[objnamez];

            objname(n1, sl->sl_key);
            fprintf(of, "%*s(\"", indent + 2, "");
            for (p = n1; *p != '\0'; ++p)
            {
                if (*p != '\"' && *p != '\\')
                {
                    fputc(*p, of);
                }
            }
            fprintf(of, "\") = ");
            write_outfile(of, profilecallof(sl->sl_value), indent + 2);
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
profile_return()
{
    /* Is this the return immediately after the call to profile()? */
    if (ici_prof_cur_call == nullptr)
    {
        /* Yes, create the top-level profilecall object. */
        ici_prof_cur_call = profilecall_new(nullptr);
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
        if (ici_prof_cur_call->pc_calledby == nullptr)
        {
            /* Yeah we've run out, end of profile. */
            #ifdef _WIN32
                timeEndPeriod(1);
            #endif

            if (ici_prof_done_callback != nullptr)
            {
                /* A callback has been supplied, this will know how to
                 * display or save the data. */
                ici_prof_done_callback(ici_prof_cur_call);
            }

            if (strlen(ici_prof_outfile) > 0)
            {
                /* A path has been supplied to save the data. */
                FILE *of = fopen(ici_prof_outfile, "w");
                if (of != nullptr)
                {
                    fputs("auto profile = ", of);
                    write_outfile(of, ici_prof_cur_call, 0);
                    fputs(";\n", of);
                    fclose(of);
                }
            }

            /* No more profiling. */
            decref(ici_prof_cur_call);
            ici_prof_cur_call = nullptr;
            profile_active = 0;
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

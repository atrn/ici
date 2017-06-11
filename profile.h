// -*- mode:c++ -*-

#ifndef ICI_PROFILE_H
#define ICI_PROFILE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

typedef struct ici_profilecall ici_profilecall_t;
extern int ici_profile_active;
void ici_profile_call(ici_func_t *f);
void ici_profile_return();
void ici_profile_set_done_callback(void (*)(ici_profilecall_t *));
ici_profilecall_t *ici_profilecall_new(ici_profilecall_t *called_by);


/*
 * Type used to store profiling call graph.
 *
 * One of these objects is created for each function called from within another
 * of these.  If a function is called more than once from the same function
 * then the object is reused.
 */
struct ici_profilecall : object
{
    ici_profilecall_t   *pc_calledby;
    ici_struct_t    *pc_calls;
    long            pc_total;
    long            pc_laststart;
    long            pc_call_count;
};
/*
 * pc_caller        The records for the function that called this function.
 * pc_calls         Records for functions called by this function.
 * pc_total         The total number of milliseconds spent in this call
 *                  so far.
 * pc_laststart     If this is currently being called then this is the
 *                  time (in milliseconds since some fixed, but irrelevent,
 *                  point) it started.  We use this at the time the call
 *                  returns and add the difference to pc_total.
 * pc_call_count    The number of times this function was called.
 */

inline ici_profilecall_t *ici_profilecallof(ici_obj_t *o) { return static_cast<ici_profilecall_t *>(o); }
inline bool ici_isprofilecall(ici_obj_t *o) { return o->isa(ICI_TC_PROFILECALL); }

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_PROFILE_H */

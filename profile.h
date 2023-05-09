// -*- mode:c++ -*-

#ifndef ICI_PROFILE_H
#define ICI_PROFILE_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * Type used to store profiling call graph.
 *
 * One of these objects is created for each function called from within another
 * of these.  If a function is called more than once from the same function
 * then the object is reused.
 */
struct profilecall : object
{
    profilecall *pc_calledby;
    map         *pc_calls;
    long         pc_total;
    long         pc_laststart;
    long         pc_call_count;
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

inline profilecall *profilecallof(object *o)
{
    return o->as<profilecall>();
}
inline bool isprofilecall(object *o)
{
    return o->hastype(TC_PROFILECALL);
}

extern int   profile_active;
void         profile_call(func *f);
void         profile_return();
void         profile_set_done_callback(void (*)(profilecall *));
profilecall *profilecall_new(profilecall *called_by);

/*
 * End of ici.h export. --ici.h-end--
 */
class profilecall_type : public type
{
public:
    profilecall_type() : type("profile call", sizeof(struct profilecall))
    {
    }

    size_t mark(object *o) override;
};

} // namespace ici

#endif /* ICI_PROFILE_H */

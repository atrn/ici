#define ICI_CORE
#include "fwd.h"
#include "types.h"
#include "array.h"
#include "buf.h"
#include "catcher.h"
#include "channel.h"
#include "exec.h"
#include "file.h"
#include "float.h"
#include "forall.h"
#include "func.h"
#include "cfunc.h"
#include "handle.h"
#include "int.h"
#include "mark.h"
#include "mem.h"
#include "method.h"
#include "op.h"
#include "parse.h"
#include "pc.h"
#include "profile.h"
#include "ptr.h"
#include "re.h"
#include "set.h"
#include "src.h"
#include "str.h"
#include "map.h"

namespace ici
{

/*
 * The array of known types. Initialised with the types known to the
 * core. NB: The positions of these must exactly match the TC_* values
 * in object.h.
 */
type *types[max_types] =
{
    nullptr,
    singleton<pc_type>(),
    singleton<src_type>(),
    singleton<parse_type>(),
    singleton<op_type>(),
    singleton<string_type>(),
    singleton<catcher_type>(),
    singleton<forall_type>(),
    singleton<int_type>(),
    singleton<float_type>(),
    singleton<regexp_type>(),
    singleton<ptr_type>(),
    singleton<array_type>(),
    singleton<map_type>(),
    singleton<set_type>(),
    singleton<exec_type>(),
    singleton<file_type>(),
    singleton<func_type>(),
    singleton<cfunc_type>(),
    singleton<method_type>(),
    singleton<mark_type>(),
    singleton<null_type>(),
    singleton<handle_type>(),
    singleton<mem_type>(),
#ifndef NOPROFILE
    singleton<profilecall_type>(),
#else
    nullptr,
#endif
    nullptr, // TC_REF is special, a reserved type code with no actual type
    singleton<channel_type>(),
    nullptr
};

static int ntypes = TC_MAX_CORE + 1;

/*
 * Register a new 'type' and return a new small int type code to use
 * in the header of objects of that type. The pointer 't' passed to
 * this function is retained and assumed to remain valid indefinetly
 * (it is normally a statically initialised structure).
 *
 * Returns the new type code, or zero on error in which case error
 * has been set.
 *
 * This --func-- forms part of the --ici-api--.
 */
int register_type(type *t)
{
    if (ntypes == max_types)
    {
        set_error("too many types");
        return 0;
    }
    types[ntypes] = t;
    return ntypes++;
}

} // namespace ici

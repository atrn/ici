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
    instanceof<pc_type>(),
    instanceof<src_type>(),
    instanceof<parse_type>(),
    instanceof<op_type>(),
    instanceof<string_type>(),
    instanceof<catcher_type>(),
    instanceof<forall_type>(),
    instanceof<int_type>(),
    instanceof<float_type>(),
    instanceof<regexp_type>(),
    instanceof<ptr_type>(),
    instanceof<array_type>(),
    instanceof<map_type>(),
    instanceof<set_type>(),
    instanceof<exec_type>(),
    instanceof<file_type>(),
    instanceof<func_type>(),
    instanceof<cfunc_type>(),
    instanceof<method_type>(),
    instanceof<mark_type>(),
    instanceof<null_type>(),
    instanceof<handle_type>(),
    instanceof<mem_type>(),
#ifndef NOPROFILE
    instanceof<profilecall_type>(),
#else
    nullptr,
#endif
    nullptr, // TC_REF is special, a reserved type code with no actual type
    instanceof<channel_type>(),
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

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
    single_instance_of<pc_type>(),
    single_instance_of<src_type>(),
    single_instance_of<parse_type>(),
    single_instance_of<op_type>(),
    single_instance_of<string_type>(),
    single_instance_of<catcher_type>(),
    single_instance_of<forall_type>(),
    single_instance_of<int_type>(),
    single_instance_of<float_type>(),
    single_instance_of<regexp_type>(),
    single_instance_of<ptr_type>(),
    single_instance_of<array_type>(),
    single_instance_of<map_type>(),
    single_instance_of<set_type>(),
    single_instance_of<exec_type>(),
    single_instance_of<file_type>(),
    single_instance_of<func_type>(),
    single_instance_of<cfunc_type>(),
    single_instance_of<method_type>(),
    single_instance_of<mark_type>(),
    single_instance_of<null_type>(),
    single_instance_of<handle_type>(),
    single_instance_of<mem_type>(),
#ifndef NOPROFILE
    single_instance_of<profilecall_type>(),
#else
    nullptr,
#endif
    nullptr, // TC_REF is special, a reserved type code with no actual type
    single_instance_of<channel_type>(),
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

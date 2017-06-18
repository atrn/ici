#define ICI_CORE
#include "fwd.h"
#include "types.h"
#include "archive.h"
#include "array.h"
#include "buf.h"
#include "catch.h"
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
#include "restorer.h"
#include "saver.h"
#include "set.h"
#include "src.h"
#include "str.h"
#include "struct.h"

namespace ici
{

/*
 * The array of known types. Initialised with the types known to the
 * core. NB: The positions of these must exactly match the ICI_TC_* defines
 * in object.h.
 */
type *types[max_types] =
{
    nullptr,
    ptr_to_instance_of<pc_type>(),
    ptr_to_instance_of<src_type>(),
    ptr_to_instance_of<parse_type>(),
    ptr_to_instance_of<op_type>(),
    ptr_to_instance_of<string_type>(),
    ptr_to_instance_of<catch_type>(),
    ptr_to_instance_of<forall_type>(),
    ptr_to_instance_of<int_type>(),
    ptr_to_instance_of<float_type>(),
    ptr_to_instance_of<regexp_type>(),
    ptr_to_instance_of<ptr_type>(),
    ptr_to_instance_of<array_type>(),
    ptr_to_instance_of<struct_type>(),
    ptr_to_instance_of<set_type>(),
    ptr_to_instance_of<exec_type>(),
    ptr_to_instance_of<file_type>(),
    ptr_to_instance_of<func_type>(),
    ptr_to_instance_of<cfunc_type>(),
    ptr_to_instance_of<method_type>(),
    ptr_to_instance_of<mark_type>(),
    ptr_to_instance_of<null_type>(),
    ptr_to_instance_of<handle_type>(),
    ptr_to_instance_of<mem_type>(),
#ifndef NOPROFILE
    ptr_to_instance_of<profilecall_type>(),
#else
    nullptr,
#endif
    ptr_to_instance_of<archive_type>(),
    nullptr, // ICI_TC_REF is special, a reserved type code with no actual type
    ptr_to_instance_of<restorer_type>(),
    ptr_to_instance_of<saver_type>(),
    ptr_to_instance_of<channel_type>()
};

static int ntypes = ICI_TC_MAX_CORE + 1;

/*
 * Register a new 'type' and return a new small int type code to use
 * in the header of objects of that type. The pointer 't' passed to
 * this function is retained and assumed to remain valid indefinetly
 * (it is normally a statically initialised structure).
 *
 * Returns the new type code, or zero on error in which case ici_error
 * has been set.
 *
 * This --func-- forms part of the --ici-api--.
 */
int register_type(type *t)
{
    if (ntypes == max_types)
    {
        ici_set_error("too many types");
        return 0;
    }
    types[ntypes] = t;
    return ntypes++;
}

} // namespace ici

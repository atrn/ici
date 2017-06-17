#define ICI_CORE
#include "object.h"

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
 * This template function creates a "Myer singleton" for some T and
 * returns its address. We use to create the instances of the various
 * type classes when initializing the types[] array.
 */
template <typename T> inline type *instance_of() {
    static T value;
    return &value;
}

/*
 * The array of known types. Initialised with the types known to the
 * core. NB: The positions of these must exactly match the ICI_TC_* defines
 * in object.h.
 */
type_t      *types[max_types] =
{
    nullptr,
    instance_of<pc_type>(),
    instance_of<src_type>(),
    instance_of<parse_type>(),
    instance_of<op_type>(),
    instance_of<string_type>(),
    instance_of<catch_type>(),
    instance_of<forall_type>(),
    instance_of<int_type>(),
    instance_of<float_type>(),
    instance_of<regexp_type>(),
    instance_of<ptr_type>(),
    instance_of<array_type>(),
    instance_of<struct_type>(),
    instance_of<set_type>(),
    instance_of<exec_type>(),
    instance_of<file_type>(),
    instance_of<func_type>(),
    instance_of<cfunc_type>(),
    instance_of<method_type>(),
    instance_of<mark_type>(),
    instance_of<null_type>(),
    instance_of<handle_type>(),
    instance_of<mem_type>(),
#ifndef NOPROFILE
    instance_of<profilecall_type>(),
#else
    nullptr,
#endif
    instance_of<archive_type>(),
    nullptr, // ICI_TC_REF is special, a reserved type code
    instance_of<restorer_type>(),
    instance_of<saver_type>(),
    instance_of<channel_type>()
};

static int ntypes = ICI_TC_MAX_CORE + 1;


/*
 * Register a new 'type_t' structure and return a new small int type code
 * to use in the header of objects of that type. The pointer 't' passed to
 * this function is retained and assumed to remain valid indefinetly
 * (it is normally a statically initialised structure).
 *
 * Returns the new type code, or zero on error in which case ici_error
 * has been set.
 *
 * This --func-- forms part of the --ici-api--.
 */
int
ici_register_type(type_t *t)
{
    if (ntypes == max_types)
    {
        ici_set_error("too many types");
        return 0;
    }
    types[ntypes] = t;
    return ntypes++;
}

//================================================================
//
// class type

unsigned long type::mark(ici_obj_t *o) {
    o->o_flags |= ICI_O_MARK;
    return size;
}

void type::free(ici_obj_t *o) {
    ici_nfree(o, size);
}

unsigned long type::hash(ici_obj_t *o) {
    return ICI_PTR_HASH(o);
}

int type::cmp(ici_obj_t *o1, ici_obj_t *o2) {
    return o1 != o2;
}

ici_obj_t *type::copy(ici_obj_t *o) {
    o->incref();
    return o;
}

int type::assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) {
    return assign_fail(o, k, v);
}

ici_obj_t * type::fetch(ici_obj_t *o, ici_obj_t *k) {
    return fetch_fail(o, k);
}

int type::assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *) {
    return assign_fail(o, k, v);
}

int type::fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **pv, ici_struct_t *) {
    *pv = fetch_fail(o, k);
    return 1;
}

int type::assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) {
    return assign(o, k, v);
}

ici_obj_t *
type::fetch_base(ici_obj_t *o, ici_obj_t *k) {
    return fetch(o, k);
}

ici_obj_t *type::fetch_method(ici_obj_t *o, ici_obj_t *n) {
    return nullptr;
}

int type::forall(ici_obj_t *o) {
    return 1;
}

void type::objname(ici_obj_t *, char n[ICI_OBJNAMEZ]) {
}

int type::call(ici_obj_t *, ici_obj_t *) {
    return 1;
}

ici_obj_t *type::fetch_fail(ici_obj_t *o, ici_obj_t *k)
{
    char n1[ICI_OBJNAMEZ];
    char n2[ICI_OBJNAMEZ];
    ici_set_error("attempt to read %s keyed by %s",
        ici_objname(n1, o),
        ici_objname(n2, k));
    return NULL;
}

int type::assign_fail(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    char n1[ICI_OBJNAMEZ];
    char n2[ICI_OBJNAMEZ];
    char n3[ICI_OBJNAMEZ];

    return ici_set_error("attempt to set %s keyed by %s to %s",
        ici_objname(n1, o),
        ici_objname(n2, k),
        ici_objname(n3, v));
}

}

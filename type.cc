#define ICI_CORE
#include "object.h"

namespace ici
{

unsigned long type::mark(ici_obj_t *o) {
    o->o_flags |= ICI_O_MARK;
    return typesize();
}

void type::free(ici_obj_t *o) {
    ici_nfree(o, typesize());
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

ici_obj_t *type::fetch_base(ici_obj_t *o, ici_obj_t *k) {
    return fetch(o, k);
}

ici_obj_t *type::fetch_method(ici_obj_t *o, ici_obj_t *n) {
    return nullptr;
}

int type::forall(ici_obj_t *o) {
    return 1;
}

void type::objname(ici_obj_t *, char n[ICI_OBJNAMEZ]) {
    n[0] = 0;
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

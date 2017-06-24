#define ICI_CORE
#include "object.h"

namespace ici
{

size_t type::mark(object *o) {
    o->setmark();
    return typesize();
}

void type::free(object *o) {
    ici_nfree(o, typesize());
}

unsigned long type::hash(object *o) {
    return ICI_PTR_HASH(o);
}

int type::cmp(object *o1, object *o2) {
    return o1 != o2;
}

object *type::copy(object *o) {
    o->incref();
    return o;
}

int type::assign(object *o, object *k, object *v) {
    return assign_fail(o, k, v);
}

object * type::fetch(object *o, object *k) {
    return fetch_fail(o, k);
}

int type::assign_super(object *o, object *k, object *v, ici_struct *) {
    return assign_fail(o, k, v);
}

int type::fetch_super(object *o, object *k, object **pv, ici_struct *) {
    *pv = fetch_fail(o, k);
    return 1;
}

int type::assign_base(object *o, object *k, object *v) {
    return assign(o, k, v);
}

object *type::fetch_base(object *o, object *k) {
    return fetch(o, k);
}

object *type::fetch_method(object *, object *) {
    return nullptr;
}

int type::forall(object *) {
    return 1;
}

void type::objname(object *, char n[objnamez]) {
    snprintf(n, objnamez, "%s %p", name, (void *)this);
}

int type::call(object *, object *) {
    return 1;
}

object *type::fetch_fail(object *o, object *k)
{
    char n1[objnamez];
    char n2[objnamez];
    set_error("attempt to read %s keyed by %s",
        ici_objname(n1, o),
        ici_objname(n2, k));
    return NULL;
}

int type::assign_fail(object *o, object *k, object *v)
{
    char n1[objnamez];
    char n2[objnamez];
    char n3[objnamez];

    return set_error("attempt to set %s keyed by %s to %s",
        ici_objname(n1, o),
        ici_objname(n2, k),
        ici_objname(n3, v));
}

}

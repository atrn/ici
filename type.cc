#define ICI_CORE
#include "object.h"

namespace ici
{

size_t type::mark(object *o) {
    o->setmark();
    return objectsize();
}

void type::free(object *o) {
    ici_nfree(o, objectsize());
}

unsigned long type::hash(object *o) {
    return ICI_PTR_HASH(o);
}

int type::cmp(object *o1, object *o2) {
    return o1 != o2;
}

object *type::copy(object *o) {
    incref(o);
    return o;
}

int type::assign(object *o, object *k, object *v) {
    return assign_fail(o, k, v);
}

object * type::fetch(object *o, object *k) {
    return fetch_fail(o, k);
}

int type::assign_super(object *o, object *k, object *v, map *) {
    return assign_fail(o, k, v);
}

int type::fetch_super(object *o, object *k, object **pv, map *) {
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

int type::forall(object *o) {
    char n[objnamez];
    return set_error("attempt to forall over %s", ici::objname(n, o));
}

void type::objname(object *, char n[objnamez]) {
    snprintf(n, objnamez, "%s %p", name, (void *)this);
}

int type::call(object *o, object *) {
    char n[objnamez];
    return set_error("attempt to call %s", ici::objname(n, o));
}

object *type::fetch_fail(object *o, object *k)
{
    char n1[objnamez], n2[objnamez];
    set_error("attempt to read %s keyed by %s",
              ici::objname(n1, o),
              ici::objname(n2, k));
    return nullptr;
}

int type::assign_fail(object *o, object *k, object *v)
{
    char n1[objnamez], n2[objnamez], n3[objnamez];
    return set_error("attempt to set %s keyed by %s to %s",
                     ici::objname(n1, o),
                     ici::objname(n2, k),
                     ici::objname(n3, v));
}

int type::save(archiver *, object *o) {
    return set_error("attempt to save a %s", o->icitype()->name);
}

object *type::restore(archiver *) {
    set_error("attempt to restore a %s", name);
    return nullptr;
}

}

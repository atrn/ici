#define ICI_CORE
#include "object.h"
#include "str.h"

namespace ici
{

type::~type()
{
}

size_t type::mark(object *o)
{
    o->setmark();
    return objectsize();
}

void type::free(object *o)
{
    ici_nfree(o, objectsize());
}

unsigned long type::hash(object *o)
{
    return ICI_PTR_HASH(o);
}

int type::cmp(object *o1, object *o2)
{
    return o1 != o2;
}

object *type::copy(object *o)
{
    incref(o);
    return o;
}

int type::assign(object *o, object *k, object *v)
{
    return assign_fail(o, k, v);
}

object *type::fetch(object *o, object *k)
{
    return fetch_fail(o, k);
}

int type::assign_super(object *o, object *k, object *v, map *)
{
    return assign_fail(o, k, v);
}

int type::fetch_super(object *o, object *k, object **pv, map *)
{
    *pv = fetch_fail(o, k);
    return 1;
}

int type::assign_base(object *o, object *k, object *v)
{
    return assign(o, k, v);
}

object *type::fetch_base(object *o, object *k)
{
    return fetch(o, k);
}

object *type::fetch_method(object *, object *)
{
    return nullptr;
}

int type::forall(object *o)
{
    char n[objnamez];
    return set_error("attempt to forall over %s", ici::objname(n, o));
}

int type::nkeys(object *)
{
    return 0;
}

int type::keys(object *o, array *)
{
    char n[objnamez];
    return set_error("attempt to obtains keys from a value of type %s", ici::objname(n, o));
}

void type::objname(object *, char n[objnamez])
{
    snprintf(n, objnamez, "[%s %p]", name, (void *)this);
}

int type::call(object *o, object *)
{
    char n[objnamez];
    return set_error("attempt to call %s", ici::objname(n, o));
}

void type::uninit()
{
    if (_name != nullptr)
    {
        _name->decref();
    }
}

object *type::fetch_fail(object *o, object *k)
{
    char n1[objnamez], n2[objnamez];
    set_error("attempt to read %s keyed by %s", ici::objname(n1, o), ici::objname(n2, k));
    return nullptr;
}

int type::assign_fail(object *o, object *k, object *v)
{
    char n1[objnamez], n2[objnamez], n3[objnamez];
    return set_error("attempt to set %s keyed by %s to %s", ici::objname(n1, o), ici::objname(n2, k),
                     ici::objname(n3, v));
}

int type::save_fail(archiver *, object *o)
{
    return set_error("attempt to save a %s", o->icitype()->name);
}

object *type::restore_fail(const char *name)
{
    set_error("attempt to restore a %s", name);
    return nullptr;
}

int type::save(archiver *ar, object *o)
{
    return save_fail(ar, o);
}

object *type::restore(archiver *)
{
    return restore_fail(name);
}

int64_t type::len(object *)
{
    return 1;
}

} // namespace ici

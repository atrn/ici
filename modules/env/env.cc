/*
 * The env module provides access the process environment via
 * a map-like global variable, 'env'.
 *
 * Environment variables values are retrieved by indexing 'env' with
 * the name of the variable, e.g.
 *
 *      printf("%s\n", env.PATH);
 *
 * Setting a variable is done vi assignment to the env member, e.g.
 *
 *      env.LD_LIBRARY_PATH += ":/usr/local/lib/ici";
 *
 * The env global is a map-like value that restricts all keys and
 * values to strings. When first created the "map" is populated from
 * the process environment and any changes are propogated to the
 * process environment.
 *
 * This --intro-- and --synopsis-- are part of --ici-env-- documentation.
 */

#include <fwd.h>
#include <alloc.h>
#include <forall.h>
#include <array.h>
#include <map.h>
#include <null.h>
#include <str.h>

namespace
{

struct env : ici::object {
    ici::map *map;
};

class env_type : public ici::map_type
{
public:
    static int tcode;
    static ici::type *instance();

    size_t mark(ici::object *) override;
    ici::object * fetch(ici::object *o, ici::object *k) override;
    int assign(ici::object *o, ici::object *k, ici::object *v) override;
    int forall(ici::object *) override;
    int64_t len(ici::object *) override;
    int nkeys(ici::object *) override;
    int keys(ici::object *, ici::array *) override;
};

inline env * envof(ici::object *o) {
    return static_cast<env *>(o);
}

// inline bool isenv(ici::object *o) {
//     return o->isa(env_type::tcode);
// }

int env_type::tcode = 0;

ici::type *env_type::instance() {
    static env_type t;
    return &t;
}

size_t env_type::mark(ici::object *o) {
    auto e = envof(o);
    auto mem = ici::type::mark(e);
    return mem + e->map->mark();
}

ici::object *env_type::fetch(ici::object *o, ici::object *k) {
    if (!ici::isstring(k)) {
        ici::set_error("attempt to use a %s key to index env", k->type_name());
        return nullptr;
    }
    return envof(o)->map->fetch(k);
}

int env_type::assign(ici::object *o, ici::object *k, ici::object *v) {
    if (!ici::isstring(k)) {
        ici::set_error("attempt to use a %s key to index env", k->type_name());
	return 1;
    }

    if (!ici::isstring(v)) {
        ici::set_error("attempt to use a %s value to set env", v->type_name());
	return 1;
    }

    if (setenv(ici::stringof(k)->s_chars, ici::stringof(v)->s_chars, 1)) {
        ici::set_error("setenv: %s", strerror(errno));
        return 1;
    }

    if (envof(o)->map->assign(k, v)) {
        unsetenv(ici::stringof(k)->s_chars);
	return 1;
    }

    return 0;
}

int env_type::forall(ici::object *o) {
    auto fa = forallof(o);
    auto e = envof(fa->fa_aggr);
    fa->fa_aggr = e->map;
    return e->map->forall(o);
}

//

env * new_env()
{
    auto o = ici::ici_talloc<env>();
    if (!o)
        return nullptr;

    o->map = ici::new_map();
    if (!o->map) {
        ici::ici_free(o);
        return nullptr;
    }
    set_tfnz(o, env_type::tcode, 0, 1, 0);
    rego(o);
    return o;
}


int64_t env_type::len(ici::object *o) {
    auto e = envof(o);
    return e->map->objlen();
}

int env_type::nkeys(ici::object *o) {
    return len(o);
}

int env_type::keys(ici::object *o, ici::array *a) {
    auto m = envof(o)->map;
    for (auto sl = m->s_slots; sl < m->s_slots + m->s_nslots; ++sl) {
	if (sl->sl_key != nullptr) {
	    if (a->push_back(sl->sl_key)) {
		return 1;
	    }
	}
    }
    return 0;
}

} // anon

extern "C" ici::object * ici_env_init() {
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "env"))
        return nullptr;

    env_type::tcode = ici::register_type(env_type::instance());
    if (!env_type::tcode)
	return nullptr;

    env *e;
    if ((e = new_env()) == nullptr)
	return nullptr;

    extern char **environ;
    for (auto p = environ; *p; ++p) {
	const auto eq = strchr(*p, '=');
	if (!eq) {
            ici::set_error("bad environ entry: \"%s\"", *p);
	    return nullptr;
	}

        ici::ref<> k = ici::new_str(*p, eq - *p);
        if (!k)
	    return nullptr;

        ici::ref<> v = ici::new_str_nul_term(eq+1);
        if (!v)
            return nullptr;

        if (e->assign(k, v))
            return nullptr;
    }

    return e;
}

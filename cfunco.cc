#define ICI_CORE
#include "fwd.h"
#include "cfunc.h"
#include "archiver.h"
#include "debugger.h"
#include "exec.h"
#include "ptr.h"
#include "map.h"
#include "op.h"
#include "pc.h"
#include "int.h"
#include "str.h"
#include "catcher.h"
#include "buf.h"
#include "mark.h"
#include "null.h"
#include "primes.h"
#ifndef NOPROFILE
#include "profile.h"
#endif

namespace ici
{

cfunc::cfunc() // sentinel for end of cfunc lists
    : object(TC_CFUNC, 0, 0, 0)
    , cf_name(nullptr)
    , cf_cfunc(nullptr)
    , cf_arg1(nullptr)
    , cf_arg2(nullptr)
{
}

/*
 * Create a new cfunc.  This is not common, because cfuncs are almost always
 * defined statically.  The name must be a static null terminated string, the
 * pointer will be retained.  The func is the C function and arg1 and arg2 are
 * assigned to cf_arg1 and cf_arg2 respectively.  The returned object has a
 * refernce of 1.
 */
cfunc *new_cfunc(str *name, int (*func)(...), void *arg1, void *arg2)
{
    cfunc *cf;

    if ((cf = ici_talloc(cfunc)) == nullptr)
        return nullptr;
    set_tfnz(cf, TC_CFUNC, 0, 1, sizeof (cfunc));
    cf->cf_name = name;
    cf->cf_cfunc = func;
    cf->cf_arg1 = arg1;
    cf->cf_arg2 = arg2;
    rego(cf);
    return cf;
}

/*
 * Assign into the structure 's' all the intrinsic functions listed in the
 * array of 'cfunc' structures pointed to by 'cf'.  The array must be
 * terminated by an entry with a 'cf_name' of nullptr.  Typically, entries in the
 * array are formated as:
 *
 *  ICI_DEFINE_CFUNC(func, f_func),
 *
 * 'func', a string object, is the name your function will be assigned to in
 * the given struct, and 'f_func' is a C function obeying the rules of ICI
 * intrinsic functions.
 *
 * Returns non-zero on error, in which case error is set, else zero.
 *
 * This --func-- forms part of the --ici-api--.
 */
int assign_cfuncs(objwsup *s, cfunc *cf)
{
    while (cf->cf_name != nullptr)
    {
        /* ### should be a decref here? ### */
        assert(ici_fetch_base(s, cf->cf_name) == null);
        if (ici_fetch_base(s, cf->cf_name) != null)
        {
            fprintf(stderr, "WARNING: duplicate builtin function '%s'\n", cf->cf_name->s_chars);
        }
        if (ici_assign_base(s, cf->cf_name, cf))
        {
            cf->cf_name->decref();
            return 1;
        }
        cf->cf_name->decref();
        ++cf;
    }
    return 0;
}

/*
 * Define the given intrinsic functions in the current static scope.
 * See assign_cfuncs() for details.
 *
 * Returns non-zero on error, in which case error is set, else zero.
 *
 * This --func-- forms part of the --ici-api--.
 */
int define_cfuncs(cfunc *cf)
{
    return assign_cfuncs(objwsupof(vs.a_top[-1])->o_super, cf);
}

/*
 * Create a new class struct and assign the given cfuncs into it (as in
 * assign_cfuncs()).  If 'super' is nullptr, the super of the new struct is
 * set to the outer-most writeable struct in the current scope.  Thus this is
 * a new top-level class (not derived from anything).  If super is non-nullptr,
 * it is presumably the parent class and is used directly as the super.
 * Returns nullptr on error, usual conventions.  The returned struct has an
 * incref the caller owns.
 *
 * This --func-- forms part of the --ici-api--.
 */
objwsup *new_class(cfunc *cf, objwsup *super)
{
    objwsup       *s;

    if ((s = objwsupof(new_map())) == nullptr)
        return nullptr;
    if (assign_cfuncs(s, cf))
    {
        s->decref();
        return nullptr;
    }
    if (super == nullptr && (super = outermost_writeable()) == nullptr)
        return nullptr;
    s->o_super = super;
    return s;
}

/*
 * Create a new module struct and assign the given cfuncs into it (as in
 * assign_cfuncs()).  Returns nullptr on error, usual conventions.  The
 * returned struct has an incref the caller owns.
 *
 * This --func-- forms part of the --ici-api--.
 */
objwsup *new_module(cfunc *cf)
{
    return new_class(cf, nullptr);
}

#ifdef NOTDEF
static int
call_cfunc_nodebug(object *o, object *subject)
{
    return cfuncof(o)->cf_cfunc(subject);
}
#endif

size_t cfunc_type::mark(object *o)
{
    return type::mark(o) + ici_mark(cfuncof(o)->cf_name);
}

object * cfunc_type::fetch(object *o, object *k)
{
    if (k == SS(name))
        return cfuncof(o)->cf_name;
    return null;
}

void cfunc_type::objname(object *o, char p[objnamez])
{
    const char    *n;
    n = cfuncof(o)->cf_name->s_chars;
    if (strlen(n) > objnamez - 2 - 1)
        sprintf(p, "%.*s...()", objnamez - 6, n);
    else
        sprintf(p, "%s()", n);
}

int cfunc_type::call(object *o, object *subject)
{
    if (UNLIKELY(debug_active)
#ifndef NOPROFILE
        ||
        UNLIKELY(ici_profile_active)
#endif  
    )
    {
        object       **xt;
        int             result;

        /*
         * Not all function calls that go stright to C code are complete
         * function calls in the ICI sense. Some push stuff to execute on
         * the ICI execution stack and the return will happen later by the
         * usual return mechanism. Only those that come back with the
         * execution stack at the same level are considered to be returning
         * now.
         */
        xt = xs.a_top - 1;
        result = (*cfuncof(o)->cf_cfunc)(subject);
        if (xt != xs.a_top)
            return result;
#ifndef NOPROFILE
        if (ici_profile_active)
            ici_profile_return();
#endif
        if (UNLIKELY(debug_active))
        {
            o_debug->fnresult(os.a_top[-1]);
        }
        return result;
    }
    return (*cfuncof(o)->cf_cfunc)(subject);
}

int cfunc_type::save(archiver *ar, object *o) {
    auto cf = cfuncof(o);
    if (auto p = ar->lookup(o)) {
        return ar->save_ref(p);
    }
    if (ar->save_name(o) || ar->write(int16_t(cf->cf_name->s_nchars)) || ar->write(cf->cf_name->s_chars, cf->cf_name->s_nchars)) {
        return 1;
    }
    return 0;
}

object *cfunc_type::restore(archiver *ar) {
    object *name;
    if (ar->restore_name(&name)) {
        return nullptr;
    }
    int16_t n;
    if (ar->read(n)) {
        return nullptr;
    }
    char buf[1024];
    if (size_t(n) >= sizeof buf) {
        set_error("attempt to restore %d byte cfunc", n);
        return nullptr;
    }
    if (ar->read(buf, n-1)) {
        return nullptr;
    }
    buf[n] = '\0';
    auto s = new_str(buf, n);
    if (!s) {
        return nullptr;
    }
    auto scope = mapof(vs.a_top[-1]);
    auto f = ici_fetch(scope, s);
    s->decref();
    if (f == nullptr || f == null) {
        set_error("attempt to restore unknown C function \"%s\"", buf);
        return nullptr;
    }
    if (!iscfunc(f)) {
        set_error("attempt to restore %s object as C function \"%s\"", f->icitype()->name, buf);
        return nullptr;
    }
    return f;
}

} // namespace ici

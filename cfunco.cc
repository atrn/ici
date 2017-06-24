#define ICI_CORE
#include "fwd.h"
#include "cfunc.h"
#include "exec.h"
#include "ptr.h"
#include "struct.h"
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

    if ((cf = ici_talloc(cfunc)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(cf, ICI_TC_CFUNC, 0, 1, sizeof (cfunc));
    cf->cf_name = name;
    cf->cf_cfunc = func;
    cf->cf_arg1 = arg1;
    cf->cf_arg2 = arg2;
    ici_rego(cf);
    return cf;
}

/*
 * Assign into the structure 's' all the intrinsic functions listed in the
 * array of 'cfunc' structures pointed to by 'cf'.  The array must be
 * terminated by an entry with a 'cf_name' of NULL.  Typically, entries in the
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
int ici_assign_cfuncs(objwsup *s, cfunc *cf)
{
    while (cf->cf_name != NULL)
    {
        /* ### should be a decref here? ### */
        assert(ici_fetch_base(s, cf->cf_name) == ici_null);
        if (ici_fetch_base(s, cf->cf_name) != ici_null)
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
 * See ici_assign_cfuncs() for details.
 *
 * Returns non-zero on error, in which case error is set, else zero.
 *
 * This --func-- forms part of the --ici-api--.
 */
int ici_def_cfuncs(cfunc *cf)
{
    return ici_assign_cfuncs(objwsupof(vs.a_top[-1])->o_super, cf);
}

/*
 * Create a new class struct and assign the given cfuncs into it (as in
 * ici_assign_cfuncs()).  If 'super' is NULL, the super of the new struct is
 * set to the outer-most writeable struct in the current scope.  Thus this is
 * a new top-level class (not derived from anything).  If super is non-NULL,
 * it is presumably the parent class and is used directly as the super.
 * Returns NULL on error, usual conventions.  The returned struct has an
 * incref the caller owns.
 *
 * This --func-- forms part of the --ici-api--.
 */
objwsup *ici_class_new(cfunc *cf, objwsup *super)
{
    objwsup       *s;

    if ((s = objwsupof(new_struct())) == NULL)
        return NULL;
    if (ici_assign_cfuncs(s, cf))
    {
        s->decref();
        return NULL;
    }
    if (super == NULL && (super = outermost_writeable_struct()) == NULL)
        return NULL;
    s->o_super = super;
    return s;
}

/*
 * Create a new module struct and assign the given cfuncs into it (as in
 * ici_assign_cfuncs()).  Returns NULL on error, usual conventions.  The
 * returned struct has an incref the caller owns.
 *
 * This --func-- forms part of the --ici-api--.
 */
objwsup *ici_module_new(cfunc *cf)
{
    return ici_class_new(cf, NULL);
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
    return ici_null;
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
    if (UNLIKELY(ici_debug_active)
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
        if (UNLIKELY(ici_debug_active))
        {
            debugfunc->idbg_fnresult(os.a_top[-1]);
        }
        return result;
    }
    return (*cfuncof(o)->cf_cfunc)(subject);
}

} // namespace ici

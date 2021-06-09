// -*- mode:c++ -*-

#ifndef ICI_CFUNC_H
#define ICI_CFUNC_H

#include "object.h"
#include "array.h"
#include "int.h"
#include "str.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct which is the ICI intrinsic function type. That is,
 * a function that is coded in C. (There are actually two types, this
 * one, and a second for functions that are coded in ICI, that are both
 * called 'func'.)
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct cfunc : object
{
    str         *cf_name;
    int         (*cf_cfunc)(...);
    const void  *cf_arg1;
    const void  *cf_arg2;

    cfunc();

    template <typename F>
    cfunc(str *name, F *f, void *arg1 = nullptr, void *arg2 = nullptr)
        : object(TC_CFUNC)
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1(arg1)
        , cf_arg2(arg2)
    {
    }
};

/*
 * 'cfunc' objects are often declared staticly (in an array) when
 * setting up a group of C functions to be called from ICI. When doing
 * this, the macro 'ICI_CF_OBJ' can be used as the initialiser of the
 * 'object' header.
 *
 * The type has a well-known built-in type code of 'TC_CFUNC'.
 *
  * cf_name              A name for the function. Calls to functions
 *                      such as 'assign_cfuncs' will use this as
 *                      the name to use when assigning it into an ICI
 *                      struct. Apart from that, it is only used in
 *                      error messages.
 *
 * cf_func()            The implementation of the function.  The formals are
 *                      not mentioned here deliberately as implementaions will
 *                      vary in their use of them.
 *
 * cf_arg1, cf_arg2     Optional additional data items.  Sometimes it is
 *                      useful to write a single C function that masquerades
 *                      as severl ICI functions - driven by distinguishing
 *                      data from these two fields. See 'ICI_CF_ARG1()'.
 *
 * This comment is also part of the --ici-api--.
 */

inline cfunc *cfuncof(object *o) { return o->as<cfunc>(); }
inline bool iscfunc(object *o) { return o->hastype(TC_CFUNC); }

/*
 * The operand stack on entry to an intrinsic function:
 *
 * arg(n-1) ... arg(1) arg(0) NARGS FUNC
 *                                        ^-os.a_top
 *
 * NARGS is an ICI int and FUNC is the function object (us).
 */

/*
 * In a call from ICI to a function coded in C, this macro returns the object
 * passed as the 'i'th actual parameter (the first parameter is ARG(0)).  The
 * type of the result is an '(object *)'.  There is no actual or implied
 * incref associated with this.  Parameters are known to be on the ICI operand
 * stack, and so can be assumed to be referenced and not garbage collected.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline object *& ARG(int n) { return os.a_top[-3 - (n)]; }

/*
 * In a call from ICI to a function coded in C, this macro returns the
 * count of actual arguments to this C function.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline int NARGS() { return int(intof(os.a_top[-2])->i_value); }

/*
 * In a call from ICI to a function coded in C, this macro returns
 * a pointer to the first argument to this function, with subsequent
 * arguments being available by *decrementing* the pointer.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline object **ARGS() { return &os.a_top[-3]; }

/*
 * In a call from ICI to a function coded in C, this macro returns the
 * 'cf_arg1' field of the current C function.  The macro 'ICI_CF_ARG2()' can also
 * be used to obtain the 'cf_arg2' field. See the 'cfunc' type.
 *
 * They are both (void *) (Prior to ICI 4.0, 'ICI_CF_ARG1()' was a function
 * pointer.)
 *
 * This --func-- forms part of the --ici-api--.
 */
inline const void *ICI_CF_ARG1() { return cfuncof(os.a_top[-1])->cf_arg1; }
inline const void *ICI_CF_ARG2() { return cfuncof(os.a_top[-1])->cf_arg2; }

/*
 * Helper macros to start the definution of a static cfuncs array.
 * This is a bit of shorthand used within the interpreter to
 * help consistency.
 */
#define ICI_DEFINE_CFUNCS(NAME) ici::cfunc ici_ ## NAME ## _cfuncs[] =

/*
 * Mark the end of the initializers of a static cfuncs array.
 * This inserts the sentinel value into the array to mark
 * the end of valid cfunc entries.
 */
#define ICI_CFUNCS_END() {}

/*
 *  Expand to the name of the given "cfuncs" table.
 */
#define ICI_CFUNCS(NAME) ici_ ## NAME ## _cfuncs

/*
 * Macros to define cfuncs. Use the one appropriate for the number of
 * arguments to the cfunc.
 *
 * In the core function names are defined as static strings and
 * referenced using the SS macro (str.h).  In modules strings are
 * defined via icistr-setup.h and referenced via the ICIS macro.
 */
#ifndef ICI_MODULE_NAME
#define ICI_DEFINE_CFUNC(NAME, FUNC) {SS(NAME), (FUNC)}
#define ICI_DEFINE_CFUNC1(NAME, FUNC, ARG) {SS(NAME), (FUNC), (void *)(ARG)}
#define ICI_DEFINE_CFUNC2(NAME, FUNC, ARG1, ARG2) {SS(NAME), (FUNC), (void *)(ARG1), (void *)(ARG2)}
#else
#define ICI_DEFINE_CFUNC(NAME, FUNC) {ICIS(NAME), (FUNC)}
#define ICI_DEFINE_CFUNC1(NAME, FUNC, ARG) {ICIS(NAME), (FUNC), (void *)(ARG)}
#define ICI_DEFINE_CFUNC2(NAME, FUNC, ARG1, ARG2) {ICIS(NAME), (FUNC), (void *)(ARG1), (void *)(ARG2)}
#endif

/*
 * Macros to define methods within a cfuncs array.
 */
#define ICI_DEFINE_METHOD(NAME, FUNC) ICI_DEFINE_CFUNC(NAME, FUNC)

class cfunc_type : public type
{
public:
    cfunc_type() : type("func", sizeof (cfunc), type::has_objname | type::has_call) {}

    size_t mark(object *o) override;
    object *fetch(object *o, object *k) override;
    void objname(object *o, char p[objnamez]) override;
    int call(object *o, object *subject) override;
    int save(archiver *, object *) override;
    object *restore(archiver *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_CFUNC_H */

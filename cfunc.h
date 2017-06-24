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
    ici_str_t * cf_name;
    int         (*cf_cfunc)(...);
    const void  *cf_arg1;
    const void  *cf_arg2;

    cfunc(bool) // sentinel for end of cfunc lists
        : object{ICI_TC_CFUNC, 0, 0, 0}
        , cf_name(nullptr)
        , cf_cfunc(nullptr)
        , cf_arg1(nullptr)
        , cf_arg2(nullptr)
    {}

    template <typename F>
    cfunc(ici_str_t *name, F *f)
        : object{ICI_TC_CFUNC, 0, 1, 0}
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1(nullptr)
        , cf_arg2(nullptr)
    {
    }

    template <typename F>
    cfunc(ici_str_t *name, F *f, void *arg1)
        : object{ICI_TC_CFUNC, 0, 1, 0}
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1(arg1)
        , cf_arg2(nullptr)
    {
    }

    cfunc(ici_str_t *name, int (*f)(), long arg1)
        : object{ICI_TC_CFUNC, 0, 1, 0}
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1((const void *)arg1)
        , cf_arg2(nullptr)
    {
    }

    cfunc(ici_str_t *name, double (*f)(...), const char *arg1)
        : object{ICI_TC_CFUNC, 0, 1, 0}
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1(arg1)
        , cf_arg2(nullptr)
    {
    }

    template <typename F>
    cfunc(ici_str_t *name, F *f, void *arg1, void *arg2)
        : object{ICI_TC_CFUNC, 0, 1, 0}
        , cf_name(name)
        , cf_cfunc(reinterpret_cast<int (*)(...)>(f))
        , cf_arg1(arg1)
        , cf_arg2(arg2)
    {
    }

};

/*
 * 'ici_cfunc_t' objects are often declared staticly (in an array) when
 * setting up a group of C functions to be called from ICI. When doing
 * this, the macro 'ICI_CF_OBJ' can be used as the initialiser of the
 * 'object' header.
 *
 * The type has a well-known built-in type code of 'ICI_TC_CFUNC'.
 *
  * cf_name              A name for the function. Calls to functions
 *                      such as 'ici_assign_cfuncs' will use this as
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

inline ici_cfunc_t *ici_cfuncof(object *o) { return static_cast<ici_cfunc_t *>(o); }
inline bool ici_iscfunc(object *o) { return o->isa(ICI_TC_CFUNC); }

/*
 * The operand stack on entry to an intrinsic function:
 *
 * arg(n-1) ... arg(1) arg(0) NARGS FUNC
 *                                        ^-ici_os.a_top
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
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ARG(n)          (ici_os.a_top[-3 - (n)])
// inline object *ARG(int n) { return ici_os.a_top[-3 - (n)]; }

/*
 * In a call from ICI to a function coded in C, this macro returns the
 * count of actual arguments to this C function.
 *
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
inline int NARGS() { return intof(ici_os.a_top[-2])->i_value; }

/*
 * In a call from ICI to a function coded in C, this macro returns
 * a pointer to the first argument to this function, with subsequent
 * arguments being available by *decrementing* the pointer.
 *
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ARGS()          (&ici_os.a_top[-3])

/*
 * In a call from ICI to a function coded in C, this macro returns the
 * 'cf_arg1' field of the current C function.  The macro 'ICI_CF_ARG2()' can also
 * be used to obtain the 'cf_arg2' field. See the 'ici_cfunc_t' type.
 *
 * They are both (void *) (Prior to ICI 4.0, 'ICI_CF_ARG1()' was a function
 * pointer.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_CF_ARG1()       (ici_cfuncof(ici_os.a_top[-1])->cf_arg1)
#define ICI_CF_ARG2()       (ici_cfuncof(ici_os.a_top[-1])->cf_arg2)

/*
 * Defines a 'cfuncs' array.
 */
#define ICI_DEFINE_CFUNCS(NAME) ici_cfunc_t ici_ ## NAME ## _cfuncs[] =

/*
 * Marks the end of the initializers of a cfuncs array.
 */
#define ICI_CFUNCS_END() {false}

/*
 * Macros to define cfuncs. Use the one for the number of arguments.
 */
#define ICI_DEFINE_CFUNC(NAME, FUNC) {SS(NAME), (FUNC)}
#define ICI_DEFINE_CFUNC1(NAME, FUNC, ARG) {SS(NAME), (FUNC), (void *)(ARG)}
#define ICI_DEFINE_CFUNC2(NAME, FUNC, ARG1, ARG2) {SS(NAME), (FUNC), (void *)(ARG1), (void *)(ARG2)}

/*
 * Macros to define methods within a cfuncs array.
 */
#define ICI_DEFINE_METHOD(NAME, FUNC) {SS(NAME), (int (*)(...))(FUNC)}

/*
 * End of ici.h export. --ici.h-end--
 */


class cfunc_type : public type
{
public:
    cfunc_type() : type("func", sizeof (cfunc), type::has_objname | type::has_call) {}

    size_t mark(object *o) override;
    object *fetch(object *o, object *k) override;
    void objname(object *o, char p[ICI_OBJNAMEZ]) override;
    int call(object *o, object *subject) override;
};

} // namespace ici

#endif /* ICI_CFUNC_H */

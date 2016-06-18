#ifndef ICI_CFUNC_H
#define ICI_CFUNC_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

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
struct ici_cfunc
{
    ici_obj_t   o_head;
    const char  *cf_name;
    int         (*cf_cfunc)();
    void        *cf_arg1;
    void        *cf_arg2;
};
/*
 * 'ici_cfunc_t' objects are often declared staticly (in an array) when
 * setting up a group of C functions to be called from ICI. When doing
 * this, the macro 'ICI_CF_OBJ' can be used as the initialiser of the
 * 'o_head' field (the standard ICI object heade).
 *
 * The type has a well-known built-in type code of 'ICI_TC_CFUNC'.
 *
 * o_head               The standard ICI object header.
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
#define ici_cfuncof(o)      ((ici_cfunc_t *)(o))
#define ici_iscfunc(o)      (ici_objof(o)->o_tcode == ICI_TC_CFUNC)

/*
 * Convienience macro for the object header for use in static
 * initialisations of ici_cfunc_t objects.
 */
#define ICI_CF_OBJ          {ICI_TC_CFUNC, 0, 1, 0}

/*
 * The operand stack on entry to an intrinsic function:
 *
 * arg(n-1) ... arg(1) arg(0) ICI_NARGS FUNC
 *                                        ^-ici_os.a_top
 *
 * ICI_NARGS is an ICI int and FUNC is the function object (us).
 */

/*
 * In a call from ICI to a function coded in C, this macro returns the object
 * passed as the 'i'th actual parameter (the first parameter is ICI_ARG(0)).  The
 * type of the result is an '(ici_obj_t *)'.  There is no actual or implied
 * incref associated with this.  Parameters are known to be on the ICI operand
 * stack, and so can be assumed to be referenced and not garbage collected.
 *
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_ARG(n)          (ici_os.a_top[-3 - (n)])

/*
 * In a call from ICI to a function coded in C, this macro returns the
 * count of actual arguments to this C function.
 *
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_NARGS()         ((int)ici_intof(ici_os.a_top[-2])->i_value)

/*
 * In a call from ICI to a function coded in C, this macro returns
 * a pointer to the first argument to this function, with subsequent
 * arguments being available by *decrementing* the pointer.
 *
 * (This macro has no ICI_ prefix for historical reasons.)
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ICI_ARGS()          (&ici_os.a_top[-3])

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
 * End of ici.h export. --ici.h-end--
 */

#define ICI_CF_ARG(X)       ((void *)(X))

#define ICI_DEFINE_CFUNCS(NAME) ici_cfunc_t ici_ ## NAME ## _cfuncs[] =
#define ICI_DEFINE_CFUNC(NAME, FUNC) {ICI_CF_OBJ, (char *)SS(NAME), (FUNC)}
#define ICI_DEFINE_CFUNC1(NAME, FUNC, ARG) {ICI_CF_OBJ, (char *)SS(NAME), (FUNC), (void *)(ARG), 0}

#endif /* ICI_CFUNC_H */

// -*- mode:c++ -*-

#ifndef ici_userop_h
#define ici_userop_h

#include "object.h"
#include "func.h"

namespace ici
{
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * Define a user-defined, binary, operator, a function that will
 * be called to implement an operator defined by the types and
 * operator given as arguments.
 *
 * It is not possible to implement defined operators, e.g. "int + int"
 * (an implementation artefact, user operators being used when other
 * operator combinations have not been matched).
 *
 * Arguments are:
 *
 *      type1   The ici type name of the LHS
 *      binop   The operaor string (ref binop_name() [arith.cc])
 *      type2   The ici type name of the RHS
 *      fn      The function called to perform the operation.
 *
 * Operator functions are called with the two "sides" as arguments,
 * left then right as per-convention. The function's result is the
 * result of the operator, e.g. an add function would be,
 *
 *      add := [func (a, b) { reutrn a + b: }];
 *
 * Returns 0 for success, non-zero for failure, usual conventions.
 */
int     define_user_binop(const char *, const char *, const char *, func *);

/*
 * Lookup a user-defined operator given the type1/binop/type2 arguments
 * used to define it. Returns nullptr if no function is found.
 */
func *  lookup_user_binop(const char *, const char *, const char *);

/*
 * Call the user-defined binop function and return the result
 * or nullptr upon failure, usual conventions.
 */
object *call_user_binop(func *, object *, object *);

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

#define ICI_CORE
#include "exec.h"
#include "map.h"
#include "int.h"
#include "float.h"
#include "buf.h"
#include "file.h"
#include "str.h"

namespace ici
{

/*
 * set_val(scope, name, typespec, value)
 *
 * Set the value of the given name in given struct to the given raw C value,
 * which is first converted to an ICI value on the assumption that it is of the
 * type specified by the given typespec. Somewhat unusually, this function
 * always sets the value in the base struct. It avoids writing in super-structs.
 * Note that the C value is generally passed as a pointer to the value.
 *
 * Type specification is similar to other internal functions (ici_func,
 * typecheck etc..) and uses the following key-letters
 *
 * Type Letter  ICI type        C type (of value)
 * ================================================
 *      i       int             long *
 *      f       float           double *
 *      s       string          char *
 *      u       file            FILE *
 *      o       any             object *
 *
 * Returns 0 on succcess, else non-zerro, usual conventions.
 */
int set_val(objwsup *s, str *name, int type, void *vp) {
    object   *o;

    switch (type) {
    case 'i':
        o = new_int(*(long *)vp);
        break;

    case 'f':
        o = new_float(*(double *)vp);
        break;
        
    case 's':
        o = new_str_nul_term((char *)vp);
        break;

    case 'u':
        o = new_file((char *)vp, stdio_ftype, name, nullptr);
        o->set(file::noclose);
        break;

    case 'o':
        o = (object *)vp;
        incref(o); /* so can decref below */
        break;

    default:
        return set_error("illegal type key-letter given to set_val");
    }

    if (o == nullptr) {
        return 1;
    }
    auto rc = ici_assign_base(s, name, o);
    decref(o);
    return rc;
}

/*
 * Utility function to form an error message when a fetch which expects to
 * find a particular value or type of value fails to do so. Returns 1 so
 * it can be used directly in typical error returns.
 */
int ici_fetch_mismatch(object *o, object *k, object *v, const char *expected)
{
    char        n1[objnamez];
    char        n2[objnamez];
    char        n3[objnamez];

    return set_error("read %s from %s keyed by %s, but expected %s",
        objname(n1, v),
        objname(n2, o),
        objname(n3, k),
        expected);
}

int ici_assign_float(object *o, object *k, double v)
{
    ici_float  *f;

    if ((f = new_float(v)) == nullptr)
        return 1;
    if (ici_assign(o, k, f))
        return 1;
    decref(f);
    return 0;
}

/*
 * Fetch a float or int from the given object keyed by the given key.
 * The result is stored as a double through the given pointer. Returns
 * non-zero on error, usual conventions.
 */
int fetch_num(object *o, object *k, double *vp)
{
    object   *v;

    if ((v = ici_fetch(o, k)) == nullptr)
        return 1;
    if (isint(v))
        *vp = intof(v)->i_value;
    else if (isfloat(v))
        *vp = floatof(v)->f_value;
    else
        return ici_fetch_mismatch(o, k, v, "a number");
    return 0;
}

/*
 * Fetch an int (a C long) from the given object keyed by the given key.
 * The result is stored as a long through the given pointer. Returns
 * non-zero on error, usual conventions.
 */
int fetch_int(object *o, object *k, long *vp)
{
    object   *v;

    if ((v = ici_fetch(o, k)) == nullptr)
        return 1;
    if (!isint(v))
        return ici_fetch_mismatch(o, k, v, "an int");
    *vp = intof(v)->i_value;
    return 0;
}

/*
 * cmkvar(scope, name, typespec, value)
 *
 * This function is a simple way to define variables (to the ICI level).
 * It takes a scope structure, name for the variable, a type specification
 * (see below) and the variable's initial value and creates a variable in
 * the specified scope.
 *
 * Type specification is similar to other internal functions (ici_func,
 * typecheck etc..) and uses the following key-letters
 *
 * Type Letter  ICI type        C type (of value)
 * ================================================
 *      i       int             long *
 *      f       float           double *
 *      s       string          char *
 *      u       file            FILE *
 *      o       any             object *
 */
int cmkvar(objwsup *scope, const char *name, int type, void *vp)
{
    str   *s;
    int         i;

    if ((s = new_str_nul_term(name)) == nullptr)
        return 1;
    i = set_val(scope, s, type, vp);
    decref(s);
    return i;
}

} // namespace ici

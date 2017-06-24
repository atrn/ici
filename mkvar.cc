#define ICI_CORE
#include "exec.h"
#include "struct.h"
#include "int.h"
#include "float.h"
#include "buf.h"
#include "file.h"
#include "str.h"

namespace ici
{

/*
 * ici_set_val(scope, name, typespec, value)
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
int ici_set_val(objwsup *s, str *name, int type, void *vp)
{
    object   *o;
    int       i;

    switch (type)
    {
    case 'i':
        o = ici_int_new(*(long *)vp);
        break;

    case 'f':
        o = ici_float_new(*(double *)vp);
        break;
        
    case 's':
        o = ici_str_new_nul_term((char *)vp);
        break;

    case 'u':
        o = ici_file_new((char *)vp, stdio_ftype, name, NULL);
        o->set(ICI_F_NOCLOSE);
        break;

    case 'o':
        o = (object *)vp;
        o->incref(); /* so can decref below */
        break;

    default:
        return set_error("illegal type key-letter given to ici_set_val");
    }

    if (o == NULL)
        return 1;
    i = ici_assign_base(s, name, o);
    o->decref();
    return i;
}

/*
 * Utility function to form an error message when a fetch which expects to
 * find a particular value or type of value fails to do so. Returns 1 so
 * it can be used directly in typical error returns.
 */
int
ici_fetch_mismatch(object *o, object *k, object *v, const char *expected)
{
    char        n1[30];
    char        n2[30];
    char        n3[30];

    return set_error("read %s from %s keyed by %s, but expected %s",
        ici_objname(n1, v),
        ici_objname(n2, o),
        ici_objname(n3, k),
        expected);
}

int
ici_assign_float(object *o, object *k, double v)
{
    ici_float  *f;

    if ((f = ici_float_new(v)) == NULL)
        return 1;
    if (ici_assign(o, k, f))
        return 1;
    return 0;
}

/*
 * Fetch a float or int from the given object keyed by the given key.
 * The result is stored as a double through the given pointer. Returns
 * non-zero on error, usual conventions.
 */
int
ici_fetch_num(object *o, object *k, double *vp)
{
    object   *v;

    if ((v = ici_fetch(o, k)) == NULL)
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
int
ici_fetch_int(object *o, object *k, long *vp)
{
    object   *v;

    if ((v = ici_fetch(o, k)) == NULL)
        return 1;
    if (!isint(v))
        return ici_fetch_mismatch(o, k, v, "an int");
    *vp = intof(v)->i_value;
    return 0;
}

/*
 * ici_cmkvar(scope, name, typespec, value)
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
int ici_cmkvar(objwsup *scope, const char *name, int type, void *vp)
{
    str   *s;
    int         i;

    if ((s = ici_str_new_nul_term(name)) == NULL)
        return 1;
    i = ici_set_val(scope, s, type, vp);
    s->decref();
    return i;
}

} // namespace ici

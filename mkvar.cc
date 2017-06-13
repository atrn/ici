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
 * ici_typecheck etc..) and uses the following key-letters
 *
 * Type Letter  ICI type        C type (of value)
 * ================================================
 *      i       int             long *
 *      f       float           double *
 *      s       string          char *
 *      u       file            FILE *
 *      o       any             ici_obj_t *
 *
 * Returns 0 on succcess, else non-zerro, usual conventions.
 */
int
ici_set_val(ici_objwsup_t *s, ici_str_t *name, int type, void *vp)
{
    ici_obj_t   *o;
    int         i;

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
        o = ici_file_new((char *)vp, &ici_stdio_ftype, name, NULL);
        o->o_flags |= ICI_F_NOCLOSE;
        break;

    case 'o':
        o = (ici_obj_t *)vp;
        o->incref(); /* so can decref below */
        break;

    default:
        return ici_set_error("illegal type key-letter given to ici_set_val");
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
ici_fetch_mismatch(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, const char *expected)
{
    char        n1[30];
    char        n2[30];
    char        n3[30];

    return ici_set_error("read %s from %s keyed by %s, but expected %s",
        ici_objname(n1, v),
        ici_objname(n2, o),
        ici_objname(n3, k),
        expected);
}

int
ici_assign_float(ici_obj_t *o, ici_obj_t *k, double v)
{
    ici_float_t  *f;

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
ici_fetch_num(ici_obj_t *o, ici_obj_t *k, double *vp)
{
    ici_obj_t   *v;

    if ((v = ici_fetch(o, k)) == NULL)
        return 1;
    if (ici_isint(v))
        *vp = ici_intof(v)->i_value;
    else if (ici_isfloat(v))
        *vp = ici_floatof(v)->f_value;
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
ici_fetch_int(ici_obj_t *o, ici_obj_t *k, long *vp)
{
    ici_obj_t   *v;

    if ((v = ici_fetch(o, k)) == NULL)
        return 1;
    if (!ici_isint(v))
        return ici_fetch_mismatch(o, k, v, "an int");
    *vp = ici_intof(v)->i_value;
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
 * ici_typecheck etc..) and uses the following key-letters
 *
 * Type Letter  ICI type        C type (of value)
 * ================================================
 *      i       int             long *
 *      f       float           double *
 *      s       string          char *
 *      u       file            FILE *
 *      o       any             ici_obj_t *
 */
int
ici_cmkvar(ici_objwsup_t *scope, const char *name, int type, void *vp)
{
    ici_str_t   *s;
    int         i;

    if ((s = ici_str_new_nul_term(name)) == NULL)
        return 1;
    i = ici_set_val(scope, s, type, vp);
    s->decref();
    return i;
}

} // namespace ici

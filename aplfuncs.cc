#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "struct.h"
#include "buf.h"
#include "null.h"
#include "op.h"
#include "array.h"

/*
 * This structure holds the recursion-independent parameters for the recursive
 * buildxx() function.
 */
struct context
{
    ici_obj_t   **c_dlimit;
    int         c_dstep;
    char        c_option;
    ici_obj_t   **c_cstart;
    ici_obj_t   **c_climit;
    ici_obj_t   **c_cnext;
    long        c_ccount;
    long        c_cstep;
};
/*
 * c_dlimit             Addr of 1st dimension we don't use.
 *
 * c_dstep              Direction to step dnext by. +/-1.
 *
 * c_option             The option char.
 *
 * c_cstart             The first element of the content.
 *
 * c_climit             Addr of 1st content we don't use.
 *
 * c_cnext              Next content waiting to be used.
 *
 * c_ccount             Count for auto increment or array index for array
 *                      content.
 *
 * c_cstep              Direction to step c_cnext by +/-1, or, if we are
 *                      doing an "i" type auto increment, the step value.
 */

/*
 * Build a data structure according to the given dimensions and content.
 * The returned object has been ici_incref()ed.
 *
 * r is a pointer through which to store the resulting object.
 * dnext is a pointer to the dimension of interest to this call.
 * c is a pointer to a struct context containing parameters that are
 * independent of the recursion. See above.
 */
static int
buildxx(ici_obj_t **r, ici_obj_t **dnext, struct context *c)
{
    int         i;
    char        n1[30];
    char        n2[30];

    if (dnext == c->c_dlimit)
    {
        /*
         * We have run out of dimensions. Time to return an element of
         * the content. We must then step our content context in accordance
         * with the supplied option.
         */
        switch (c->c_option)
        {
        case 'i':
            if ((*r = ici_objof(ici_int_new(c->c_ccount))) == NULL)
                return 1;
            c->c_ccount += c->c_cstep;
            break;

        case 'a':
            if (!ici_isarray(*c->c_cnext))
            {
                return ici_set_error(
                    "build(..\"a\"..) given %s instead of an array for content",
                    ici_objname(n1, *c->c_cnext));
            }
            *r = ici_array_get(ici_arrayof(*c->c_cnext), c->c_ccount);
            ici_incref(*r);
            c->c_cnext += c->c_cstep;
            break;

        default:
            *r = *c->c_cnext;
            ici_incref(*r);
            c->c_cnext += c->c_cstep;
        }
        switch (c->c_option)
        {
        case 'i':
            break;

        case '\0':
        case 'a':
        case 'c':
        case 'r': /* See end of function for restart case. */
            if (c->c_cnext == c->c_climit)
            {
                c->c_cnext = c->c_cstart;
                ++c->c_ccount;
            }
            break;

        case 'l':
            if (c->c_cnext == c->c_climit)
                c->c_cnext -= c->c_cstep;
            break;

        default:
            return ici_set_error("option \"%c\" given to %s is not one of c, r, a, i or l",
                c->c_option, ici_objname(n1, ici_os.a_top[-1]));
        }
        return 0;
    }
    if (ici_isint(*dnext))
    {
        ici_array_t     *a;
        int             n;

        /*
         * We have an int dimension. We must make an array that big and
         * recursively fill it based on the next dimension or content.
         */
        n = ici_intof(*dnext)->i_value;
        if ((a = ici_array_new(n)) == NULL)
            return 1;
        for (i = 0; i < n; ++i)
        {
            if (buildxx(a->a_top, dnext + c->c_dstep, c))
            {
                ici_decref(a);
                return 1;
            }
            ++a->a_top;
            ici_decref(a->a_top[-1]);
        }
        *r = ici_objof(a);
    }
    else if (ici_isarray(*dnext))
    {
        ici_array_t     *a;
        ici_struct_t    *s;
        ici_obj_t       **e;
        ici_obj_t       *o;

        /*
         * We have an array dimension. This means a struct with the elements
         * of the array as keys. We must recursively build the struct elememts
         * with the next dimension or content.
         */
        a = ici_arrayof(*dnext);
        if ((s = ici_struct_new()) == NULL)
            return 1;
        for (e = ici_astart(a); e != ici_alimit(a); e = ici_anext(a, e))
        {
            if (buildxx(&o, dnext + c->c_dstep, c))
            {
                ici_decref(s);
                return 1;
            }
            if (ici_assign(s, *e, o))
            {
                ici_decref(s);
                return 1;
            }
            ici_decref(o);
        }
        *r = ici_objof(s);
    }
    else
    {
        return ici_set_error("%s supplied as a dimension to %s",
            ici_objname(n1, *dnext), ici_objname(n2, ici_os.a_top[-1]));
    }
    if (c->c_option == 'r')
        c->c_cnext = c->c_cstart;
    return 0;
}

static int
f_build(...)
{
    ici_obj_t           **dstart;
    int                 i;
    ici_obj_t           *r;
    ici_obj_t           *default_content;
    char                n1[30];
    struct context      c;

    memset(&c, 0, sizeof c);
    dstart = &ICI_ARG(0);
    c.c_dlimit = &ICI_ARG(ICI_NARGS()); /* Assume for the moment. */
    c.c_dstep = -1;
    for (i = 0; i < ICI_NARGS(); ++i)
    {
        if (ici_isstring(ICI_ARG(i)))
        {
            c.c_dlimit = &ICI_ARG(i); /* Revise. */
            c.c_option = ici_str_char_at(ici_stringof(ICI_ARG(i)), 0);
            if (++i < ICI_NARGS())
            {
                c.c_cstart = &ICI_ARG(i);
                c.c_climit = &ICI_ARG(ICI_NARGS());
                c.c_cstep = -1;
            }
            break;
        }
    }
    if (dstart == c.c_dlimit)
        return ici_null_ret();
    if (c.c_cstart == NULL)
    {
        default_content = ici_null;
        c.c_cstart = &default_content;
        c.c_climit = c.c_cstart + 1;
        c.c_cstep = 1;
    }
    c.c_cnext = c.c_cstart;

    if (c.c_option == 'i')
    {
        if (c.c_cnext != c.c_climit && c.c_cnext != &default_content)
        {
            if (!ici_isint(*c.c_cnext))
            {
                return ici_set_error("%s given as auto-increment start is not an int",
                    ici_objname(n1, *c.c_cnext));
            }
            c.c_ccount = ici_intof(*c.c_cnext)->i_value;
            c.c_cnext += c.c_cstep;
            if (c.c_cnext != c.c_climit)
            {
                if (!ici_isint(*c.c_cnext))
                {
                    return ici_set_error("%s given as auto-increment step is not an int",
                        ici_objname(n1, *c.c_cnext));
                }
                c.c_cstep = ici_intof(*c.c_cnext)->i_value;
            }
            else
                c.c_cstep = 1;
        }
        else
            c.c_cstep = 1;
    }

    if (buildxx(&r, dstart, &c))
        return 1;
    return ici_ret_with_decref(r);
}

ICI_DEFINE_CFUNCS(apl)
{
    ICI_DEFINE_CFUNC(build, f_build),
    ICI_CFUNCS_END
};

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

namespace ici
{

/*
 * This structure holds the recursion-independent parameters for the recursive
 * buildxx() function.
 */
struct context
{
    object   **c_dlimit;
    int         c_dstep;
    char        c_option;
    object   **c_cstart;
    object   **c_climit;
    object   **c_cnext;
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
buildxx(object **r, object **dnext, struct context *c)
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
            if ((*r = new_int(c->c_ccount)) == NULL)
                return 1;
            c->c_ccount += c->c_cstep;
            break;

        case 'a':
            if (!isarray(*c->c_cnext))
            {
                return set_error(
                    "build(..\"a\"..) given %s instead of an array for content",
                    objname(n1, *c->c_cnext));
            }
            *r = arrayof(*c->c_cnext)->get(c->c_ccount);
            (*r)->incref();
            c->c_cnext += c->c_cstep;
            break;

        default:
            *r = *c->c_cnext;
            (*r)->incref();
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
            return set_error("option \"%c\" given to %s is not one of c, r, a, i or l",
                c->c_option, objname(n1, os.a_top[-1]));
        }
        return 0;
    }
    if (isint(*dnext))
    {
        array     *a;
        int       n;

        /*
         * We have an int dimension. We must make an array that big and
         * recursively fill it based on the next dimension or content.
         */
        n = intof(*dnext)->i_value;
        if ((a = new_array(n)) == NULL)
            return 1;
        for (i = 0; i < n; ++i)
        {
            if (buildxx(a->a_top, dnext + c->c_dstep, c))
            {
                a->decref();
                return 1;
            }
            ++a->a_top;
            a->a_top[-1]->decref();
        }
        *r = a;
    }
    else if (isarray(*dnext))
    {
        array      *a;
        ici_struct *s;
        object     **e;
        object     *o;

        /*
         * We have an array dimension. This means a struct with the elements
         * of the array as keys. We must recursively build the struct elememts
         * with the next dimension or content.
         */
        a = arrayof(*dnext);
        if ((s = new_struct()) == NULL)
            return 1;
        for (e = a->astart(); e != a->alimit(); e = a->anext(e))
        {
            if (buildxx(&o, dnext + c->c_dstep, c))
            {
                s->decref();
                return 1;
            }
            if (ici_assign(s, *e, o))
            {
                s->decref();
                return 1;
            }
            o->decref();
        }
        *r = s;
    }
    else
    {
        return set_error("%s supplied as a dimension to %s",
            objname(n1, *dnext), objname(n2, os.a_top[-1]));
    }
    if (c->c_option == 'r')
        c->c_cnext = c->c_cstart;
    return 0;
}

static int
f_build(...)
{
    object         **dstart;
    int              i;
    object          *r;
    object          *default_content;
    char             n1[30];
    struct context   c;

    memset(&c, 0, sizeof c);
    dstart = &ARG(0);
    c.c_dlimit = &ARG(NARGS()); /* Assume for the moment. */
    c.c_dstep = -1;
    for (i = 0; i < NARGS(); ++i)
    {
        if (isstring(ARG(i)))
        {
            c.c_dlimit = &ARG(i); /* Revise. */
            c.c_option = str_char_at(stringof(ARG(i)), 0);
            if (++i < NARGS())
            {
                c.c_cstart = &ARG(i);
                c.c_climit = &ARG(NARGS());
                c.c_cstep = -1;
            }
            break;
        }
    }
    if (dstart == c.c_dlimit)
        return null_ret();
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
            if (!isint(*c.c_cnext))
            {
                return set_error("%s given as auto-increment start is not an int",
                    objname(n1, *c.c_cnext));
            }
            c.c_ccount = intof(*c.c_cnext)->i_value;
            c.c_cnext += c.c_cstep;
            if (c.c_cnext != c.c_climit)
            {
                if (!isint(*c.c_cnext))
                {
                    return set_error("%s given as auto-increment step is not an int",
                        objname(n1, *c.c_cnext));
                }
                c.c_cstep = intof(*c.c_cnext)->i_value;
            }
            else
                c.c_cstep = 1;
        }
        else
            c.c_cstep = 1;
    }

    if (buildxx(&r, dstart, &c))
        return 1;
    return ret_with_decref(r);
}

ICI_DEFINE_CFUNCS(apl)
{
    ICI_DEFINE_CFUNC(build, f_build),
    ICI_CFUNCS_END()
};

} // namespace ici

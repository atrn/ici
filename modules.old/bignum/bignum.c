#define ICI_NO_OLD_NAMES

/*
 * DEC/INRIA big integer arithmetic
 *
 * ICI language binding to the DEC/INRIA "bignum" high-precision interger
 * arithmetic package
 *
 *
 * This --intro-- and --synopsis-- are part of --ici-bignum-- documentation.
 */

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

#include "BigNum.h"
#include "BigZ.h"

static BigZ     zero;

static void
ici_bignum_pre_free(ici_handle_t *h)
{
    BzFree((BigZ)h->h_ptr);
}

static ici_handle_t *
new_bignum(BigZ z)
{
    ici_handle_t *h;

    if ((h = ici_handle_new(z, ICIS(bignum), NULL)) != NULL)
    {
        h->h_pre_free = ici_bignum_pre_free;
        ici_objof(h)->o_flags &= ~ICI_O_SUPER;
    }
    return h;
}

/*
 * Called to set error when we fail to allocate a bignum.
 */
static int
allocerr(void)
{
    ici_error = "failed to allocate a BigNum";
    return 1;
}

/*
 * bignum = bignum.bignum(int|string)
 *
 * Convert an int or string to a bignum. The string may contain leading
 * spaces, followed by an optional sign, followed by a series of digits.
 *
 * This --topic-- forms part of the --ici-bignum-- documentation.
 */
static int
ici_bignum_bignum(void)
{
    BigZ        z;

    if (ICI_NARGS() > 1)
        return ici_argcount(1);
    if (ICI_NARGS() == 0)
        z = BzFromInteger(0);
    else
    {
        if (ici_isstring(ICI_ARG(0)))
            z = BzFromString(ici_stringof(ICI_ARG(0))->s_chars, 10);
        else if (ici_isint(ICI_ARG(0)))
            z = BzFromInteger(ici_intof(ICI_ARG(0))->i_value);
        else
        {
            char n[80];

            ici_objname(n, ICI_ARG(0));
            sprintf(ici_buf, "unable to convert a %s to a bignum", n);
            ici_error = ici_buf;
            return 1;
        }
    }
    if (z == NULL)
        return allocerr();
    return ici_ret_with_decref(ici_objof(new_bignum(z)));
}

/* 
 * bignum = bignum.create(ndigits)
 *
 * Create a bignum with room to store 'ndigits' digits.
 *
 * This --topic-- forms part of the --ici-bignum-- documentation.
 */
static int
ici_bignum_create(void)
{
    long        ndigits;
    BigZ        z;

    if (ici_typecheck("i", &ndigits))
        return 1;
    if (ndigits < 1)
    {
        ici_error = "attempt to create bignum with zero (or fewer) digits";
        return 1;
    }
    if ((z = BzCreate(ndigits)) == NULL)
        return allocerr();
    return ici_ret_with_decref(ici_objof(new_bignum(z)));
}

/* 
 * string = bignum.tostring(bignum)
 *
 * Return a textual representation of 'bignum'.
 *
 * This --topic-- forms part of the --ici-bignum-- documentation.
 */
static int
ici_bignum_tostring(void)
{
    ici_handle_t        *bn;
    char                *s;
    ici_str_t           *str;

    if (ici_typecheck("h", ICIS(bignum), &bn))
        return 1;
    s = BzToString((BigZ)bn->h_ptr, 10);
    str = ici_str_new_nul_term(s);
    BzFreeString(s);
    return ici_ret_with_decref(ici_objof(str));
}

static int
glue_N_N(void)
{
    ici_handle_t *a;
    BigZ (*pf)(BigZ);
    BigZ z;

    if (ici_typecheck("h", ICIS(bignum), &a))
        return 1;
    pf = ICI_CF_ARG1();
    z = (*pf)((BigZ)a->h_ptr);
    return ici_ret_with_decref(ici_objof(new_bignum(z)));
}

static int
glue_NN_N(void)
{
    ici_handle_t *a;
    ici_handle_t *b;
    BigZ (*pf)(BigZ, BigZ);
    BigZ z;

    if (ici_typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    pf = ICI_CF_ARG1();
    z = (*pf)((BigZ)a->h_ptr, (BigZ)b->h_ptr);
    return ici_ret_with_decref(ici_objof(new_bignum(z)));
}

/*
 * bignum = bignum.div(bignum, bignum)
 *
 * Return the bignum floor of dividing the first bignum by the second.
 *
 * This --topic-- forms part of the --ici-bignum-- documentation.
 */
static int
ici_bignum_div(void)
{
    ici_handle_t *a;
    ici_handle_t *b;
    BigZ z;

    if (ici_typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    if (BzCompare((BigZ)b->h_ptr, zero) == 0)
    {
        ici_error = "division by zero";
        return 1;
    }
    z = BzDiv((BigZ)a->h_ptr, (BigZ)b->h_ptr);
    return ici_ret_with_decref(ici_objof(new_bignum(z)));
}

/*
 * int = bignum.compare(bignum, bignum)
 *
 * Return -1, 0, or 1 depending if the first bignum is less than,
 * equal to, or greater than the second.
 *
 * This --topic-- forms part of the --ici-bignum-- documentation.
 */
static int
ici_bignum_compare(void)
{
    ici_handle_t *a;
    ici_handle_t *b;

    if (ici_typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    return ici_int_ret(BzCompare((BigZ)a->h_ptr, (BigZ)b->h_ptr));
}

static ici_cfunc_t ici_bignum_cfuncs[] =
{
    {ICI_CF_OBJ, "bignum",   ici_bignum_bignum},
    {ICI_CF_OBJ, "create",   ici_bignum_create},
    {ICI_CF_OBJ, "tostring", ici_bignum_tostring},
    {ICI_CF_OBJ, "div",      ici_bignum_div},
    {ICI_CF_OBJ, "compare",  ici_bignum_compare},

    /*
     * bignum = bignum.negate(bignum)
     *
     * Return the negative of the given bignum.
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "negate",   glue_N_N,  BzNegate},

    /*
     * bignum = bignum.abs(bignum)
     *
     * Return the absolute value of the given bignum.
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "abs",      glue_N_N,  BzAbs},

    /*
     * bignum = bignum.add(bignum, bignum)
     *
     * Return the sum of the two bignums.
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "add",      glue_NN_N, BzAdd},

    /*
     * bignum = bignum.sub(bignum, bignum)
     *
     * Return the bignum result of subtracting the second bignum
     * from the first.
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "sub",      glue_NN_N, BzSubtract},

    /*
     * bignum = bignum.mult(bignum, bignum)
     *
     * Return the product of the two bignums.
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "mult",     glue_NN_N, BzMultiply},

    /*
     * bnr = bignum.mod(bna, bnb)
     *
     * Return the positive remainder from dividing the bignum 'bna'
     * by the bignum 'bnb'. That is:
     *
     *  0 <= bignum.mod(bna, bnb) < |bnb|
     *
     * and
     *
     *  bna == bignum.div(bna, bnb) * bnb + bignum.mod(bna, bnb)
     *
     * This --topic-- forms part of the --ici-bignum-- documentation.
     */
    {ICI_CF_OBJ, "mod",      glue_NN_N, BzMod},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_bignum_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "bignum"))
        return NULL;
    if (init_ici_str())
        return NULL;
    BzInit();
    zero = BzFromInteger(0);
    return ici_objof(ici_module_new(ici_bignum_cfuncs));
}

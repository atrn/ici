#define ICI_MODULE_NAME bignum

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>

extern "C" {
#include "BigNum.h"
#include "BigZ.h"
}

namespace {

BigZ zero;

void bignum_pre_free(ici::handle *h)
{
    BzFree((BigZ)h->h_ptr);
}

ici::handle *new_bignum(BigZ z)
{
    ici::handle *h;

    if ((h = ici::new_handle(z, ICIS(bignum), NULL)) != NULL)
    {
        h->h_pre_free = bignum_pre_free;
        h->clr(ici::object::O_SUPER);
    }
    return h;
}

int allocerr()
{
    ici::set_error("failed to allocate a BigNum");
    return 1;
}

int bignum_bignum(void)
{
    BigZ        z;

    if (ici::NARGS() > 1)
        return ici::argcount(1);
    if (ici::NARGS() == 0)
        z = BzFromInteger(0);
    else
    {
        if (ici::isstring(ici::ARG(0)))
            z = BzFromString(ici::stringof(ici::ARG(0))->s_chars, 10);
        else if (ici::isint(ici::ARG(0)))
            z = BzFromInteger(ici::intof(ici::ARG(0))->i_value);
        else
        {
            char n[80];

            ici::objname(n, ici::ARG(0));
            sprintf(ici::buf, "unable to convert a %s to a bignum", n);
            ici::set_error(ici::buf);
            return 1;
        }
    }
    if (z == NULL)
        return allocerr();
    return ici::ret_with_decref(new_bignum(z));
}

int bignum_create(void)
{
    long        ndigits;
    BigZ        z;

    if (ici::typecheck("i", &ndigits))
        return 1;
    if (ndigits < 1)
    {
        ici::set_error("attempt to create bignum with zero (or fewer) digits");
        return 1;
    }
    if ((z = BzCreate(ndigits)) == NULL)
        return allocerr();
    return ici::ret_with_decref(new_bignum(z));
}

int bignum_tostring(void)
{
    ici::handle        *bn;
    char                *s;
    ici::str            *str;

    if (ici::typecheck("h", ICIS(bignum), &bn))
        return 1;
    s = BzToString((BigZ)bn->h_ptr, 10);
    str = ici::new_str_nul_term(s);
    BzFreeString(s);
    return ici::ret_with_decref(str);
}

int glue_N_N()
{
    ici::handle *a;
    BigZ (*pf)(BigZ);
    BigZ z;

    if (ici::typecheck("h", ICIS(bignum), &a))
        return 1;
    pf = (decltype(pf))(ici::ICI_CF_ARG1());
    z = (*pf)((BigZ)a->h_ptr);
    return ici::ret_with_decref(new_bignum(z));
}

int glue_NN_N()
{
    ici::handle *a;
    ici::handle *b;
    BigZ (*pf)(BigZ, BigZ);
    BigZ z;

    if (ici::typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    pf = (decltype(pf))(ici::ICI_CF_ARG1());
    z = (*pf)((BigZ)a->h_ptr, (BigZ)b->h_ptr);
    return ici::ret_with_decref(new_bignum(z));
}

int bignum_div(void)
{
    ici::handle *a;
    ici::handle *b;
    BigZ z;

    if (ici::typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    if (BzCompare((BigZ)b->h_ptr, zero) == 0)
    {
        ici::set_error("division by zero");
        return 1;
    }
    z = BzDiv((BigZ)a->h_ptr, (BigZ)b->h_ptr);
    return ici::ret_with_decref(new_bignum(z));
}

int bignum_compare(void)
{
    ici::handle *a;
    ici::handle *b;

    if (ici::typecheck("hh", ICIS(bignum), &a, ICIS(bignum), &b))
        return 1;
    return ici::int_ret(BzCompare((BigZ)a->h_ptr, (BigZ)b->h_ptr));
}

} // anon

extern "C" ici::object *ici_bignum_init()
{
    if (ici::check_interface(ici::version_number, ici::back_compat_version, "bignum"))
        return nullptr;
    if (init_ici_str())
        return nullptr;
    BzInit();
    zero = BzFromInteger(0);
    static ICI_DEFINE_CFUNCS(bignum)
    {
        ICI_DEFINE_CFUNC(bignum,   bignum_bignum),
        ICI_DEFINE_CFUNC(create,   bignum_create),
        ICI_DEFINE_CFUNC(tostring, bignum_tostring),
        ICI_DEFINE_CFUNC(div,      bignum_div),
        ICI_DEFINE_CFUNC(compare,  bignum_compare),
        ICI_DEFINE_CFUNC1(negate,  glue_N_N,  BzNegate),
        ICI_DEFINE_CFUNC1(abs,     glue_N_N,  BzAbs),
        ICI_DEFINE_CFUNC1(add,     glue_NN_N, BzAdd),
        ICI_DEFINE_CFUNC1(sub,     glue_NN_N, BzSubtract),
        ICI_DEFINE_CFUNC1(mult,    glue_NN_N, BzMultiply),
        ICI_DEFINE_CFUNC1(mod,     glue_NN_N, BzMod),
        ICI_CFUNCS_END()
    };
    return ici::new_module(ICI_CFUNCS(bignum));
}

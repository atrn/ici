/*
 * A 64-bit integer type for ICI.
 *
 * This --intro-- formas part of the --ici-long-- documentation.
 */

#define ICI_NO_OLD_NAMES

#include <ici.h>

typedef struct
{
    ici_obj_t   o_head;
    long long   l_value;
}
    long_t;

static int tcode;

#define	longof(o)	((long_t *)(o))
#define	islong(o)	((o)->o_tcode == tcode)

static unsigned long
mark_long(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return sizeof (long_t);
}

static void
free_long(ici_obj_t *o)
{
    ici_tfree(o, sizeof (long_t));
}

static unsigned long
hash_long(ici_obj_t *o)
{
    return 17 * (longof(o)->l_value & 0xFFFFFFFF) ^ (longof(o)->l_value >> 32);
}

static int
cmp_long(ici_obj_t *a, ici_obj_t *b)
{
    return longof(a)->l_value != longof(b)->l_value;
}

ici_type_t long_type =
{
    mark_long,
    free_long,
    hash_long,
    cmp_long,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "long"
};

static long_t *
atom_long(long long int value)
{
    ici_obj_t	        *o;
    long_t              proto;

    ICI_OBJ_SET_TFNZ(&proto, tcode, 0, 1, 0);
    proto.l_value = value;
    if ((o = ici_atom_probe(ici_objof(&proto))) != NULL)
        ici_incref(o);
    return longof(o);
}

long_t *
ici_long_new_long(long long value)
{
    long_t	*l;

    if ((l = atom_long(value)) != NULL)
    {
        ici_incref(l);
        return l;
    }
    if ((l = ici_talloc(long_t)) != NULL)
    {
        ICI_OBJ_SET_TFNZ(l, tcode, 0, 1, sizeof (long_t));
        ici_rego(l);
        ici_objof(l)->o_leafz = 0;
        l->l_value = value;
        l = longof(ici_atom(ici_objof(l), 1));
    }
    return l;
}

static int
f_long(void)
{
    long		a;
    long		b;

    if (ICI_NARGS() == 0)
    {
        a = b = 0;
    }
    else if (ICI_NARGS() == 1)
    {
        if (ici_isint(ICI_ARG(0)))
            a = ici_intof(ICI_ARG(0))->i_value;
        else if (ici_isstring(ICI_ARG(0)))
        {
            long long	value;

            if (sscanf(ici_stringof(ICI_ARG(0))->s_chars, "%lld", &value) != 1)
            {
                ici_error = "invalid long value";
                return 1;
            }
            return ici_ret_with_decref(ici_objof(ici_long_new_long(value)));
        }
        else
            return ici_argerror(0);
        b = 0;
    }
    else if (ICI_NARGS() == 2)
    {
        if (ici_typecheck("ii", &b, &a))
            return 1;
    }
    else
        return ici_argcount(2);

    return ici_ret_with_decref(ici_objof(ici_long_new_long(((long long)b << 32LL) | a)));
}

static int
f_tostring(void)
{
    char	mybuf[32];

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    sprintf(mybuf, "%lld", longof(ICI_ARG(0))->l_value);
    return ici_str_ret(mybuf);
}

static int
f_tohexstring(void)
{
    char	mybuf[64];

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    sprintf(mybuf, "0x%0llX", longof(ICI_ARG(0))->l_value);
    return ici_str_ret(mybuf);
}


static int
f_negate(void)
{
    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(- longof(ICI_ARG(0))->l_value)));
}

static int
f_abs(void)
{
    long long	value;

    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    if ((value = longof(ICI_ARG(0))->l_value) < 0)
        value = -value;
    return ici_ret_with_decref(ici_objof(ici_long_new_long(value)));
}

static int
f_and(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value & b->l_value)));
}

static int
f_or(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value | b->l_value)));
}

static int
f_xor(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value ^ b->l_value)));
}

static int
f_shiftleft(void)
{
    long_t	*a;
    long	b;

    if (ici_typecheck("oi", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value << b)));
}

static int
f_shiftright(void)
{
    long_t	*a;
    long	b;

    if (ici_typecheck("oi", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value >> b)));
}

static int
f_add(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value + b->l_value)));
}

static int
f_sub(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value - b->l_value)));
}

static int
f_mult(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value * b->l_value)));
}

static int
f_div(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    if (b->l_value == 0)
    {
        ici_error = "division by zero";
        return 1;
    }
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value / b->l_value)));
}

static int
f_mod(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    if (b->l_value == 0)
    {
        ici_error = "division by zero";
        return 1;
    }
    return ici_ret_with_decref(ici_objof(ici_long_new_long(a->l_value % b->l_value)));
}

static int
f_compare(void)
{
    long_t	*a;
    long_t	*b;

    if (ici_typecheck("oo", &a, &b))
        return 1;
    if (!islong(ici_objof(a)))
        return ici_argerror(0);
    if (!islong(ici_objof(b)))
        return ici_argerror(1);
    if (a->l_value == b->l_value)
        return ici_int_ret(0);
    if (a->l_value > b->l_value)
        return ici_int_ret(1);
    return ici_int_ret(-1);
}

static int
f_top(void)
{
    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    return ici_int_ret(longof(ICI_ARG(0))->l_value >> 32);
}

static int
f_bot(void)
{
    if (ICI_NARGS() != 1)
        return ici_argcount(1);
    if (!islong(ICI_ARG(0)))
        return ici_argerror(0);
    return ici_int_ret(longof(ICI_ARG(0))->l_value & 0xFFFFFFFF);
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ, "long", f_long},
    {ICI_CF_OBJ, "tostring", f_tostring},
    {ICI_CF_OBJ, "tohexstring", f_tohexstring},
    {ICI_CF_OBJ, "negate", f_negate},
    {ICI_CF_OBJ, "abs", f_abs},
    {ICI_CF_OBJ, "and", f_and},
    {ICI_CF_OBJ, "or", f_or},
    {ICI_CF_OBJ, "xor", f_xor},
    {ICI_CF_OBJ, "shiftleft", f_shiftleft},
    {ICI_CF_OBJ, "shiftright", f_shiftright},
    {ICI_CF_OBJ, "add", f_add},
    {ICI_CF_OBJ, "sub", f_sub},
    {ICI_CF_OBJ, "mult", f_mult},
    {ICI_CF_OBJ, "div", f_div},
    {ICI_CF_OBJ, "mod", f_mod},
    {ICI_CF_OBJ, "compare", f_compare},
    {ICI_CF_OBJ, "top", f_top},
    {ICI_CF_OBJ, "bot", f_bot},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_long_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "long"))
        return NULL;
    if ((tcode = ici_register_type(&long_type)) == 0)
        return NULL;
    return ici_objof(ici_module_new(cfuncs));
}

/*
 * This --intro-- forms part of the --ici-utype-- documentation.
 */
#define ICI_NO_OLD_NAMES

#include "utype.h"
#include "icistr.h"
#include <icistr-setup.h>

int     ici_utype_tcode;
int     ici_ustruct_tcode;

static ici_ustruct_t *
ici_ustruct_new(ici_utype_t *utype)
{
    ici_ustruct_t	*u = ici_talloc(ici_ustruct_t);

    if (u != NULL)
    {
	if ((u->u_struct = ici_struct_new()) == NULL)
	{
	    ici_tfree(u, ici_ustruct_t);
	    return NULL;
	}
        ICI_OBJ_SET_TFNZ(u, ici_ustruct_tcode, 0, 1, 0);
	u->u_type = utype;
	ici_rego(ici_objof(u));
	ici_decref(u->u_struct);
    }
    return u;
}

static unsigned long
mark_ustruct(ici_obj_t *o)
{
    o->o_flags |= ICI_O_MARK;
    return
	sizeof (ici_ustruct_t)
	+
	ici_mark(ici_ustructof(o)->u_struct)
	+
	ici_mark(ici_ustructof(o)->u_type);
}

static void
free_ustruct(ici_obj_t *o)
{
    ici_tfree(o, sizeof (ici_ustruct_t));
}

static int assign_ustruct(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    ici_func_t	*f;

    if ((f = ici_ustructof(o)->u_type->u_assign) == NULL)
	return ici_assign(ici_ustructof(o)->u_struct, k, v);
    return ici_func(ici_objof(f), "ooo", ici_ustructof(o)->u_struct, k, v);
}

static ici_obj_t *
fetch_ustruct(ici_obj_t *o, ici_obj_t *k)
{
    ici_func_t	*f;
    ici_obj_t	*v;

    if ((f = ici_ustructof(o)->u_type->u_fetch) == NULL)
	return ici_fetch(ici_ustructof(o)->u_struct, k);
    if (ici_func(ici_objof(f), "o=oo", &v, ici_ustructof(o)->u_struct, k))
        return NULL;
    ici_decref(v);
    return v;
}

static ici_type_t ici_ustruct_type =
{
    mark_ustruct,
    free_ustruct,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    assign_ustruct,
    fetch_ustruct,
    "ustruct"
};

static ici_utype_t *
ici_utype_new(ici_str_t *name, ici_func_t *fnew, ici_func_t *fassign, ici_func_t *ffetch)
{
    ici_utype_t *u = ici_talloc(ici_utype_t);

    if (u != NULL)
    {
        ICI_OBJ_SET_TFNZ(u, ici_utype_tcode, 0, 1, 0);
	u->u_name   = name;
	u->u_new    = fnew;
	u->u_assign = fassign;
	u->u_fetch  = ffetch;
	u->u_type   = ici_ustruct_type;
	u->u_type.t_name = name->s_chars;
	ici_rego(ici_objof(u));
    }
    return u;
}

static unsigned long
mark_utype(ici_obj_t *o)
{
    long rc;

    o->o_flags |= ICI_O_MARK;
    rc = sizeof (ici_utype_t) + ici_mark(ici_utypeof(o)->u_name);
    if (ici_utypeof(o)->u_new != NULL)
	rc += ici_mark(ici_utypeof(o)->u_new);
    if (ici_utypeof(o)->u_assign != NULL)
	rc += ici_mark(ici_utypeof(o)->u_assign);
    if (ici_utypeof(o)->u_fetch != NULL)
	rc += ici_mark(ici_utypeof(o)->u_fetch);
    return rc;
}

static void
free_utype(ici_obj_t *o)
{
    ici_tfree(o, sizeof (ici_utype_t));
}

static ici_obj_t *
fetch_utype(ici_obj_t *o, ici_obj_t *k)
{
    ici_func_t		*f;

    if (k == ici_objof(ICIS(newx)))
	f = ici_utypeof(o)->u_new;
    else if (k == ici_objof(ICIS(assign)))
	f = ici_utypeof(o)->u_assign;
    else if (k == ici_objof(ICIS(fetch)))
	f = ici_utypeof(o)->u_fetch;
    else
	return ici_fetch_fail(o, k);
    return f == NULL ? ici_null : ici_objof(f);
}

static int
assign_utype(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v)
{
    if (!ici_isfunc(v))
	return ici_assign_fail(o, k, v);
    if (k == ici_objof(ICIS(newx)))
	ici_utypeof(o)->u_new = ici_funcof(v);
    else if (k == ici_objof(ICIS(assign)))
	ici_utypeof(o)->u_assign = ici_funcof(v);
    else if (k == ici_objof(ICIS(fetch)))
	ici_utypeof(o)->u_fetch = ici_funcof(v);
    else
	return ici_assign_fail(o, k, v);
    return 0;
}

static ici_type_t	ici_utype_type =
{
    mark_utype,
    free_utype,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    assign_utype,
    fetch_utype,
    "utype"
};

static int
f_def(void)
{
    ici_str_t	*name;
    ici_func_t	*fnew = NULL;
    ici_func_t	*fassign = NULL;
    ici_func_t	*ffetch = NULL;

    switch (ICI_NARGS())
    {
    case 1:
	if (ici_typecheck("o", &name))
	    return 1;
	break;
    case 2:
	if (ici_typecheck("oo", &name, &fnew))
	    return 1;
	break;
    case 3:
	if (ici_typecheck("ooo", &name, &fnew, &fassign))
	    return 1;
	break;
    case 4:
	if (ici_typecheck("oooo", &name, &fnew, &fassign, &ffetch))
	    return 1;
	break;
    default:
	return ici_argcount(4);
    }
    if (!ici_isstring(ici_objof(name)))
	return ici_argerror(0);
    if (fnew != NULL && !ici_isfunc(ici_objof(fnew)) && !ici_isnull(ici_objof(fnew)))
	return ici_argerror(1);
    if (fassign != NULL && !ici_isfunc(ici_objof(fassign)) && !ici_isnull(ici_objof(fassign)))
	return ici_argerror(2);
    if (ffetch != NULL && !ici_isfunc(ici_objof(ffetch)) && !ici_isnull(ici_objof(ffetch)))
	return ici_argerror(3);
    if (ici_isnull(ici_objof(fnew)))
	fnew = NULL;
    if (ici_isnull(ici_objof(fassign)))
	fassign = NULL;
    if (ici_isnull(ici_objof(ffetch)))
	ffetch = NULL;
    return ici_ret_with_decref(ici_objof(ici_utype_new(name, fnew, fassign, ffetch)));
}

static int
f_new(void)
{
    ici_utype_t         *utype;
    ici_array_t         *vargs;
    ici_obj_t           **o;
    int                 i;
    ici_ustruct_t       *self;
    int                 failed;

    o = ICI_ARGS();
    utype = ici_utypeof(*o--);
    if (!ici_isutype(ici_objof(utype)))
	return ici_argerror(0);
    if ((self = ici_ustruct_new(utype)) == NULL)
	return 1;
    if ((vargs = ici_array_new(ICI_NARGS())) == NULL)
    {
	ici_decref(self);
	return 1;
    }
    *vargs->a_top++ = ici_objof(self->u_struct);
    if ((i = ICI_NARGS() - 1) > 0)
    {
	if (ici_stk_push_chk(vargs, i+1))
	{
	    ici_decref(self);
	    ici_decref(vargs);
	    return 1;
	}
	while (i-- > 0)
	    *vargs->a_top++ = *o--;
    }
    failed = ici_call(ICIS(call), "oo", utype->u_new, vargs);
    ici_decref(vargs);
    if (failed)
    {
	ici_decref(self);
	return 1;
    }
    return ici_ret_with_decref(ici_objof(self));
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ,	"def",		f_def},
    {ICI_CF_OBJ,	"new",		f_new},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_utype_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "utype"))
	return NULL;
    if (init_ici_str())
	return NULL;
    if ((ici_utype_tcode = ici_register_type(&ici_utype_type)) == 0)
        return NULL;
    if ((ici_ustruct_tcode = ici_register_type(&ici_ustruct_type)) == 0)
        return NULL;
    return ici_objof(ici_module_new(cfuncs));
}

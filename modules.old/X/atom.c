/*
 * $Id: atom.c,v 1.2 2000/07/29 07:01:05 atrn Exp $
 */

#include "ici.h"

#include "atom.h"
#include "display.h"

static unsigned long
mark_atom(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_atom_t);
}

static unsigned long
hash_atom(object_t *o)
{
    return atomof(o)->a_atom * UNIQUE_PRIME;
}

static int
cmp_atom(object_t *a, object_t *b)
{
    return atomof(a)->a_atom != atomof(b)->a_atom;
}

type_t	ici_X_atom_type =
{
    mark_atom,
    free_simple,
    hash_atom,
    cmp_atom,
    copy_simple,
    assign_simple,
    fetch_simple,
    "atom"
};

INLINE
static object_t *
atom_atom(Atom atom)
{
    object_t	**po, *o;

    for
    (
	po = &atoms[ici_atom_hash_index(atom * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if (isatom(o) && atomof(o)->a_atom == atom)
	{
	    return o;
	}
    }
    return NULL;
}

ici_X_atom_t *
ici_X_new_atom(Atom a)
{
    object_t	*o;

    if ((o = atom_atom(a)) != NULL)
    {
	incref(o);
	return atomof(o);
    }
    if ((o = objof(talloc(ici_X_atom_t))) != NULL)
    {
	o->o_tcode = TC_OTHER;
	o->o_flags = 0;
	o->o_nrefs = 1;
	o->o_type  = &ici_X_atom_type;
	atomof(o)->a_atom = a;
	rego(o);
    }
    return o == NULL ? NULL : atomof(atom(o, 1));
}

FUNC(Atom)
{
    long		val;
    
    if (ici_typecheck("i", &val))
	return 1;
    return ici_ret_with_decref(objof(ici_X_new_atom((Atom)val)));
}

FUNC(InternAtom)
{
    ici_X_display_t	*display;
    char		*name;
    long		only_if_exists;
    Atom		atom;

    switch (NARGS())
    {
    case 2:
	if (ici_typecheck("os", &display, &name))
	    return 1;
	only_if_exists = 0;
	break;

    case 3:
	if (ici_typecheck("osi", &display, &name, &only_if_exists))
	    return 1;
	break;

    default:
	return ici_argcount(3);
    }
    atom = XInternAtom(display->d_display, name, only_if_exists);
    return ici_ret_with_decref(objof(ici_X_new_atom(atom)));
}

FUNC(GetAtomName)
{
    ici_X_display_t	*display;
    ici_X_atom_t	*atom;
    char		*name;
    string_t		*result;

    if (ici_typecheck("oo", &display, &atom))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isatom(objof(atom)))
	return ici_argcount(1);
    name = XGetAtomName(display->d_display, atom->a_atom);
    if ((result = new_cname(name)) == NULL)
    {
	XFree(name);
	return 1;
    }
    XFree(name);
    return ici_ret_with_decref(objof(result));
}

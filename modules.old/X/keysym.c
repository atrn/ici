#include "ici.h"
#include "keysym.h"
#include "event.h"

static unsigned long
mark_keysym(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_keysym_t);
}

static unsigned long
hash_keysym(object_t *o)
{
    return (unsigned long)keysymof(o)->k_keysym * UNIQUE_PRIME;
}

static int
cmp_keysym(object_t *a, object_t *b)
{
    return (a != b) || keysymof(a)->k_keysym != keysymof(b)->k_keysym;
}

type_t	ici_X_keysym_type =
{
    mark_keysym,
    free_simple,
    hash_keysym,
    cmp_keysym,
    copy_simple,
    assign_simple,
    fetch_simple,
    "keysym"
};

INLINE
static ici_X_keysym_t *
atom_keysym(KeySym keysym)
{
    object_t	**po;
    object_t	*o;

    for
    (
	po = &atoms[ici_atom_hash_index(keysym * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if
	(
	    iskeysym(o)
	    &&
	    keysymof(o)->k_keysym == keysym
	)
	{
	    return keysymof(o);
	}
    }
    return NULL;
}

ici_X_keysym_t *
ici_X_new_keysym(KeySym keysym)
{
    ici_X_keysym_t	*k;

    if ((k = atom_keysym(keysym)) != NULL)
    {
	incref(objof(k));
	return k;
    }
    if ((k = talloc(ici_X_keysym_t)) != NULL)
    {
	objof(k)->o_tcode = TC_OTHER;
	objof(k)->o_nrefs = 1;
	objof(k)->o_flags = 0;
	objof(k)->o_type  = &ici_X_keysym_type;
	rego(objof(k));
	k->k_keysym = keysym;
    }
    return k == NULL ? k : keysymof(atom(objof(k), 1));
}

FUNC(KeysymToString)
{
    ici_X_keysym_t	*k;
    char		*s;

    if (ici_typecheck("o", &k))
	return 1;
    if (!iskeysym(objof(k)))
	return ici_argerror(0);
    if ((s = XKeysymToString(k->k_keysym)) == NULL)
	return null_ret();
    return str_ret(s);
}

FUNC(LookupKeysym)
{
    ici_X_event_t	*key_event;
    long		index;
    KeySym		keysym;

    if (NARGS() == 1)
    {
	if (ici_typecheck("o", &key_event))
	    return 1;
	index = 0;
    }
    else
    {
	if (ici_typecheck("oi", &key_event, &index))
	    return 1;
    }
    if (!isevent(objof(key_event)))
	return ici_argerror(0);
    keysym = XLookupKeysym(&key_event->e_event->xkey, index);
    return ici_ret_with_decref(objof(ici_X_new_keysym(keysym)));
}

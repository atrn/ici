#ifndef ICI_X_ATOM_H
#define	ICI_X_ATOM_H

/*
 * $Id: atom.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_atom
{
    object_t	o_head;
    Atom	a_atom;
};

extern type_t		ici_X_atom_type;

#define	atomof(o)	((ici_X_atom_t *)(o))
#define	isatom(o)	((o)->o_type == &ici_X_atom_type)

ici_X_atom_t		*ici_X_new_atom(Atom);

#endif

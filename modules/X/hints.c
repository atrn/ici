/*
 */

#include <ctype.h>

#include "ici.h"
#include "display.h"
#include "window.h"
#include "atom.h"

FUNC(SetClassHint)
{
    ici_X_display_t	*display = NULL; /* To stop warning */
    ici_X_window_t	*window = NULL; /* To stop warning */
    char		*name;
    char		*class;
    int			freeclass;
    XClassHint		hint;

    freeclass = 0;
    switch (NARGS())
    {
    case 3:
	if (ici_typecheck("oos", &display, &window, &name))
	    return 1;
	if (!isdisplay(objof(display)))
	    return ici_argerror(0);
	if (!iswindow(objof(window)))
	    return ici_argerror(1);
	if ((class = ici_alloc(strlen(name + 1))) == NULL)
	    return 1;
	strcpy(class, name);
	if (islower(*class))
	    *class = toupper(*class);
	freeclass = 1;
	break;

    case 4:
	if (ici_typecheck("ooss", &display, &window, &name, &class))
	    return 1;
	if (!isdisplay(objof(display)))
	    return ici_argerror(0);
	if (!iswindow(objof(window)))
	    return ici_argerror(1);
	break;

    default:
	return ici_argcount(2);
    }
    hint.res_name = name;
    hint.res_class = class;
    XSetClassHint(display->d_display, window->w_window, &hint);
    if (freeclass)
	ici_free(class);
    return null_ret();
}

NEED_STRING(input);
NEED_STRING(state);
NEED_STRING(icon_pixmap);
NEED_STRING(icon_window);
NEED_STRING(icon_position);
NEED_STRING(icon_mask);
NEED_STRING(window_group);
NEED_STRING(urgency);
NEED_STRING(all);
NEED_STRING(withdrawn);
NEED_STRING(normal);
NEED_STRING(iconic);

FUNC(SetWMHints)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    struct_t		*s;
    slot_t		*sl;
    XWMHints		*xwmhints;
    int			n;
    
    NEED_STRINGS(1);

    if (ici_typecheck("ood", &display, &window, &s))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((xwmhints = XAllocWMHints()) == NULL)
    {
	error = "can't XAllocWMHints";
	return 1;
    }
    for (sl = s->s_slots, n = s->s_nslots; n > 0; ++sl, --n)
    {
	if (sl->sl_key == objof(STRING(input)))
	{
	    if (!isint(sl->sl_value))
	    {
		error = "bad value for \"input\" member, not an int";
		return 1;
	    }
	    xwmhints->flags |= InputHint;
	    xwmhints->input = (Bool)intof(sl->sl_value)->i_value;
	}
	else if (sl->sl_key == objof(STRING(state)))
	{
	    if (!isstring(sl->sl_value))
	    {
		error = "bad value for \"state\" member, not a string";
		return 1;
	    }
	    xwmhints->flags |= StateHint;
	    if (sl->sl_value == objof(STRING(withdrawn)))
		xwmhints->initial_state = WithdrawnState;
	    else if (sl->sl_value == objof(STRING(normal)))
		xwmhints->initial_state = NormalState;
	    else if (sl->sl_value == objof(STRING(iconic)))
		xwmhints->initial_state = IconicState;
	    else
	    {
		error = "bad value for \"state\" member, unrecognised value";
		return 1;
	    }
	}
	else if (sl->sl_key == objof(STRING(icon_pixmap)))
	{
	}
	else if (sl->sl_key == objof(STRING(icon_window)))
	{
	}
	else if (sl->sl_key == objof(STRING(icon_position)))
	{
	}
	else if (sl->sl_key == objof(STRING(icon_mask)))
	{
	}
	else if (sl->sl_key == objof(STRING(window_group)))
	{
	    xwmhints->flags |= WindowGroupHint;
	    if (iswindow(sl->sl_value))
		xwmhints->window_group = windowof(sl->sl_value)->w_window;
	    else
	    {
		error = "bad type for value of \"window_group\"";
		return 1;
	    }
	}
	else if (sl->sl_key == objof(STRING(urgency)))
	{
	}
	/* Ignore other members */
    }
    XSetWMHints(display->d_display, window->w_window, xwmhints);
    XFree(xwmhints);
    return null_ret();
}

FUNC(SetWMProtocols)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    array_t		*a;
    object_t		**po;
    Atom		*xatoms;
    Atom		*ap;
    int			n;
    
    if (ici_typecheck("ooa", &display, &window, &a))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((n = a->a_top - a->a_base) > 0)
    {
	if ((xatoms = ici_alloc(n * sizeof (Atom))) == NULL)
	{
	    error = "can't allocate space for XAtom table";
	    return 1;
	}
	for (ap = xatoms, po = a->a_base; po < a->a_top; ++po, ++ap)
	{
	    if (!isatom(*po))
	    {
		ici_free(xatoms);
		error = "non-atom in atoms array";
		return 1;
	    }
	    *ap = atomof(*po)->a_atom;
	}
	XSetWMProtocols(display->d_display, window->w_window, xatoms, n);
	ici_free(xatoms);
    }
    return null_ret();
}

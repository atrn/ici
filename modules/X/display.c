/*
 * $Id: display.c,v 1.3 2000/07/14 08:16:01 atrn Exp $
 *
 * A type to represent X11 displays and functions to operate on them.
 */

#include "ici.h"
#include "null.h"
#include "display.h"
#include "window.h"

static unsigned long
mark_display(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_display_t);
}

static unsigned long
hash_display(object_t *o)
{
    return (unsigned long)displayof(o)->d_display * UNIQUE_PRIME;
}

static int
cmp_display(object_t *a, object_t *b)
{
    return displayof(a)->d_display != displayof(b)->d_display;
}

type_t	ici_X_display_type =
{
    mark_display,
    free_simple,
    hash_display,
    cmp_display,
    copy_simple,
    assign_simple,
    fetch_simple,
    "display"
};

INLINE
static ici_X_display_t *
atom_display(Display *display)
{
    object_t	**po, *o;

    for
    (
	po = &atoms[ici_atom_hash_index((unsigned long)display * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if (isdisplay(o) && displayof(o)->d_display == display)
	{
	    return displayof(o);
	}
    }
    return NULL;
}

ici_X_display_t *
ici_X_new_display(Display *display)
{
    ici_X_display_t	*d;

    if ((d = atom_display(display)) != NULL)
    {
	incref(objof(d));
	return d;
    }
    if ((d = talloc(ici_X_display_t)) != NULL)
    {
	objof(d)->o_type  = &ici_X_display_type;
	objof(d)->o_tcode = TC_OTHER;
	objof(d)->o_flags = 0;
	objof(d)->o_nrefs = 1;
	rego(objof(d));
	d->d_display = display;
    }
    return d == NULL ? d : displayof(atom(objof(d), 1));
}

#if 0
inline
static int
checkdisplay(ici_X_display_t *display)
{
    if (display->d_display == NULL)
    {
	error = "display not open";
	return 1;
    }
    return 0;
}
#endif

FUNC(OpenDisplay)
{
    char	*name = NULL;
    object_t	*o;
    Display	*display;
    
    if (NARGS() > 0)
    {
	if (ici_typecheck("o", &o))
	    return 1;
	if (isstring(o))
	    name = stringof(o)->s_chars;
	else if (!isnull(o))
	    return ici_argerror(0);
    }
    if ((display = XOpenDisplay(name)) == NULL)
	return null_ret();
    return ici_ret_with_decref(objof(ici_X_new_display(display)));
}

FUNC(CloseDisplay)
{
    if (NARGS() != 1)
	return ici_argcount(1);
    if (!isdisplay(ARG(0)))
	return ici_argerror(0);
    if (displayof(ARG(0))->d_display != NULL)
    {
	XCloseDisplay(displayof(ARG(0))->d_display);
	displayof(ARG(0))->d_display = NULL;
    }
    return null_ret();
}

FUNC(DefaultScreen)
{
    ici_X_display_t	*display;
    
    if (ici_typecheck("o", &display))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(DefaultScreen(display->d_display));
}

FUNC(RootWindow)
{
    ici_X_display_t	*display;
    long		screen;
    ici_X_window_t	*w;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    w = ici_X_new_window(RootWindow(display->d_display, screen));
    return ici_ret_with_decref(objof(w));
}

FUNC(DefaultRootWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*w;

    if (ici_typecheck("o", &display))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    w = ici_X_new_window(DefaultRootWindow(display->d_display));
    return ici_ret_with_decref(objof(w));
}

FUNC(DisplayWidth)
{
    ici_X_display_t	*display;
    long		screen;

    if (NARGS() == 1)
    {
	if (ici_typecheck("o", &display))
	    return 1;
	screen = DefaultScreen(display->d_display);
    }
    else if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(DisplayWidth(display->d_display, screen));
}

FUNC(DisplayHeight)
{
    ici_X_display_t	*display;
    long		screen;

    if (NARGS() == 1)
    {
	if (ici_typecheck("o", &display))
	    return 1;
	screen = DefaultScreen(display->d_display);
    }
    else if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(DisplayHeight(display->d_display, screen));
}

FUNC(Flush)
{
    ici_X_display_t	*display;

    if (ici_typecheck("o", &display))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    XFlush(display->d_display);
    return null_ret();
}

FUNC(Sync)
{
    ici_X_display_t	*display;
    long		discard;

    switch (NARGS())
    {
    case 1:
	if (ici_typecheck("o", &display))
	    return 1;
	discard = 0;
	break;
    case 2:
	if (ici_typecheck("oi", &display, &discard))
	    return 1;
	break;
    default:
	return ici_argcount(2);
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    XSync(display->d_display, discard);
    return null_ret();
}

FUNC(Synchronize)
{
    ici_X_display_t	*display;
    long		onoff = 1;

    switch (NARGS())
    {
    case 1:
	if (ici_typecheck("o", &display))
	    return 1;
	onoff = 1;
	break;

    default:
	if (ici_typecheck("oi", &display, &onoff))
	    return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    XSynchronize(display->d_display, (Bool)onoff);
    return null_ret();
}

FUNC(ConnectionNumber)
{
    ici_X_display_t	*display;

    if (ici_typecheck("o", &display))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(ConnectionNumber(display->d_display));
}

static struct_t *
x_error_event_to_struct(XErrorEvent *e)
{
    struct_t		*s;
    ici_X_display_t	*d;
    long		v;

    if ((s = new_struct()) != NULL)
    {
	v = e->serial;
	if (ici_cmkvar(s, "serial", 'i', &v))
	    goto fail;
	v = e->error_code;
	if (ici_cmkvar(s, "error_code", 'i', &v))
	    goto fail;
	v = e->request_code;
	if (ici_cmkvar(s, "request_code", 'i', &v))
	    goto fail;
	v = e->minor_code;
	if (ici_cmkvar(s, "minor_code", 'i', &v))
	    goto fail;
	if ((d = ici_X_new_display(e->display)) == NULL)
	    goto fail;
	if (ici_cmkvar(s, "display", 'o', &d))
	{
	    decref(d);
	    goto fail;
	}
	decref(d);
    }
    return s;

fail:
    decref(s);
    return NULL;
}

static object_t	*ici_error_handler = NULL;

static int
error_handler(Display *d, XErrorEvent *e)
{
    ici_X_display_t	*id;
    struct_t		*ie;

    if
    (
	ici_error_handler != NULL
	&&
	(id = ici_X_new_display(d)) != NULL
	&&
	(ie = x_error_event_to_struct(e)) != NULL
    )
    {
	error = ici_func(ici_error_handler, "oo", id, ie);
	decref(id);
	decref(ie);
    }
    return 0;
}

FUNC(SetErrorHandler)
{
    object_t	*handler;
    object_t	*result;

    if (ici_typecheck("o", &handler))
	return 1;
    result = ici_error_handler;
    if (isnull(handler))
    {
	if (ici_error_handler != NULL)
	    decref(ici_error_handler);
	ici_error_handler = NULL;
    }
    else if (isfunc(handler))
    {
	if (ici_error_handler != NULL)
	    decref(ici_error_handler);
	ici_error_handler = handler;
	incref(ici_error_handler);
    }
    else
	return ici_argerror(0);
    XSetErrorHandler(error_handler);
    return ici_ret_no_decref(result);
}

FUNC(GetErrorText)
{
    ici_X_display_t	*display;
    long		code;

    if (ici_typecheck("oi", &display, &code))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (chkbuf(512))
	return 1;
    XGetErrorText(display->d_display, code, ici_buf, ici_bufz);
    return str_ret(buf);
}

FUNC(DefaultDepth)
{
    ici_X_display_t	*display;
    long		screen;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(DefaultDepth(display->d_display, screen));
}

FUNC(BlackPixel)
{
    ici_X_display_t	*display;
    long		screen;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(BlackPixel(display->d_display, screen));
}

FUNC(WhitePixel)
{
    ici_X_display_t	*display;
    long		screen;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    return int_ret(WhitePixel(display->d_display, screen));
}

FUNC(Bell)
{
    ici_X_display_t	*display;
    long		percent;

    if (NARGS() == 1)
    {
	percent = 100;
	if (ici_typecheck("o", &display))
	    return 1;
    }
    else
    {
	if (ici_typecheck("oi", &display, &percent))
	    return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    XBell(display->d_display, percent);
    return null_ret();
}

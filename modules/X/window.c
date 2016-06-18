/*
 * $Id: window.c,v 1.4 2000/07/14 08:16:01 atrn Exp $
 *
 * A type to represent X11 windows and functions that operate on them.
 */

#include "ici.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "cursor.h"
#include "event.h"
#include "colormap.h"
#include "cursor.h"

static unsigned long
mark_window(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_window_t);
}

static unsigned long
hash_window(object_t *o)
{
    return (unsigned long)windowof(o)->w_window * UNIQUE_PRIME;
}

static int
cmp_window(object_t *a, object_t *b)
{
    if (a == b)
	return 0;
    if (windowof(a)->w_window == windowof(b)->w_window)
	return 0;
    return 1;
}

type_t	ici_X_window_type =
{
    mark_window,
    free_simple,
    hash_window,
    cmp_window,
    copy_simple,
    assign_simple,
    fetch_simple,
    "window"
};

INLINE
static object_t *
atom_window(Window window)
{
    object_t	**po, *o;

    for
    (
	po = &atoms[ici_atom_hash_index(window * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if (iswindow(o) && windowof(o)->w_window == window)
	    return o;
    }
    return NULL;
}

ici_X_window_t *
ici_X_new_window(Window win)
{
    ici_X_window_t	*w;

    if ((w = windowof(atom_window(win))) != NULL)
    {
	incref(objof(w));
	return w;
    }
    if ((w = talloc(ici_X_window_t)) != NULL)
    {
	objof(w)->o_type  = &ici_X_window_type;
	objof(w)->o_tcode = TC_OTHER;
	objof(w)->o_flags = 0;
	objof(w)->o_nrefs = 1;
	rego(objof(w));
	w->w_window = win;
    }
    return w == NULL ? w : windowof(atom(objof(w), 1));
}

/**
 * CreateSimpleWindow - simplest way to make a new window
 *
 * @SYNOPSIS@ window = CreateSimpleWindow(display, ...)
 *
 * @DESCRIPTION@
 * CreateSimpleWindow is the simplest of the window creation
 * functions. It provides many convenient defauls allowing it
 * to be called with a minimum of one parameter, the display
 * on which the window should be created.
 *
 * @PARAMETERS@
 *	display		The X display on which the window should be
 *			created. This is a required parameter.
 *
 *
 */
FUNC(CreateSimpleWindow)
{
    Window		window;
    ici_X_display_t	*display;
    ici_X_window_t	*parent;
    long		w, h;
    long		bg;

    switch (NARGS())
    {
    case 2:
	if (ici_typecheck("oo", &display, &parent))
	    return 1;
	if (!isdisplay(objof(display)))
	    return ici_argerror(0);
	w = 400;
	h = 200;
	bg = WhitePixel(display->d_display, DefaultScreen(display->d_display));
	break;

    case 3:
	if (ici_typecheck("ooi", &display, &parent, &bg))
	    return 1;
	w = 400;
	h = 200;
	break;

    case 4:
	if (ici_typecheck("ooii", &display, &parent, &w, &h))
	    return 1;
	if (!isdisplay(objof(display)))
	    return ici_argerror(0);
	bg = WhitePixel(display->d_display, DefaultScreen(display->d_display));
	break;

    case 5:
	if (ici_typecheck("ooiii", &display, &parent, &w, &h, &bg))
	    return 1;
	break;

    default:
	return ici_argcount(4);
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(parent)))
	return ici_argerror(1);
    window = XCreateSimpleWindow
	     (
		 display->d_display,
		 parent->w_window,
		 0, 0,
		 w, h,
		 0,
		 BlackPixel(display->d_display,
		     DefaultScreen(display->d_display)),
		 bg
	     );
    if (!window)
    {
	error = "can't open window";
	return 1;
    }
    return ici_ret_with_decref(objof(ici_X_new_window(window)));
}

#if 0
/**
 * CreateWindow
 */
xFUNC(CreateWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*parent;
    long		x, y, d;
    long		w, h, bw;
    long		class;
    ici_X_visual_t	*visual;
    Visual		*xvisual;
    struct_t		*attr;
    unsigned long	attrmask;
    XSetWindowAttributes xattr;

    bw = x = y = 0;
    d = CopyFromParent;
    class = CopyFromParent;
    xvisual = CopyFromParent;
    attr = NULL;
    switch (NARGS())
    {
    case 4:
	if (ici_typecheck("ooii", &display, &parent, &w, &h))
	    return 1;
	break;

    case 5:
	if (ici_typecheck("ooiid", &display, &parent, &w, &h, &attr))
	    return 1;
	break;

    default:
	return ici_argcount(5);
    }
    /* XXX */
    if (attr != NULL)
	attrmask = struct_to_window_attributes(attr, &xwinattr);
    else
    {
	xattr.backing_store = Always;
	xattr.
    }
    window = XCreateWindow(display->d_display, parent, x, y, w, h, bw, d,
	class, xvisual, attrmask, &xattr);
    
}
#endif

/**
 * Window - convert objects to X11 window objects
 *
 * @SYNOPSIS@ window = Window(any)
 *
 * @DESCRIPTION@
 * This function attempts to convert the argument to a window object.
 * Valid types are converted, invalid types raise errors. Currently only
 * integers are valid (and windows which simply return themselves). It is
 * assumed these integers are derived from window identifiers listed by
 * programs such as xwininfo and the like, read into a program as text and
 * converted to integers.
 *
 * @SEE ALSO@ CreateWindow, CreateSimpleWindow
 */
FUNC(Window)
{
    ici_X_display_t	*display;
    object_t		*o;
    Window		w;

    if (ici_typecheck("oo", &display, &o))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (isint(o))
	w = (Window)intof(o)->i_value;
    else if (iswindow(o))
	return ici_ret_no_decref(o);
    else
	return ici_argerror(1);
    return ici_ret_with_decref(objof(ici_X_new_window(w)));
}

/**
 * WindowId - return the integer window identifier for a window.
 */
FUNC(WindowId)
{
    ici_X_window_t	*window;

    if (ici_typecheck("o", &window))
	return 1;
    if (!iswindow(objof(window)))
	return ici_argerror(0);
    return int_ret(window->w_window);
}

/** 
 * RaiseWindow
 */
FUNC(RaiseWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XRaiseWindow(display->d_display, window->w_window);
    return null_ret();
}

FUNC(MapWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XMapWindow(display->d_display, window->w_window);
    return null_ret();
}

FUNC(UnmapWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XUnmapWindow(display->d_display, window->w_window);
    return null_ret();
}

FUNC(UnmapSubwindows)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XUnmapSubwindows(display->d_display, window->w_window);
    return null_ret();
}

FUNC(MapRaised)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XMapRaised(display->d_display, window->w_window);
    return null_ret();
}

FUNC(MapSubwindows)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XMapSubwindows(display->d_display, window->w_window);
    return null_ret();
}

FUNC(DestroyWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XDestroyWindow(display->d_display, window->w_window);
    return null_ret();
}

FUNC(StoreName)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    char		*name;

    if (ici_typecheck("oos", &display, &window, &name))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XStoreName(display->d_display, window->w_window, name);
    return null_ret();
}

FUNC(FetchName)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    char		*name;
    
    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if (!XFetchName(display->d_display, window->w_window, &name))
	name = "";
    return str_ret(name);
}

FUNC(SelectInput)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		event_mask;
    object_t		*arg;

    if (ici_typecheck("ooo", &display, &window, &arg))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if (isint(arg))
	event_mask = intof(arg)->i_value;
    else if (isstring(arg))
	event_mask = ici_X_event_mask_for(stringof(arg));
    else if (isset(arg))
    {
	object_t **o;

	for
	(
	    event_mask = 0, o = setof(arg)->s_slots;
	    o - setof(arg)->s_slots < setof(arg)->s_nslots;
	    ++o
	)
	{
	    if (*o != NULL && isstring(*o))
		event_mask |= ici_X_event_mask_for(stringof(*o));
	}
    }
    else
	return ici_argerror(2);
    XSelectInput(display->d_display, window->w_window, event_mask);
    return null_ret();
}

NEED_STRING(width);
NEED_STRING(height);
NEED_STRING(border_width);
NEED_STRING(sibling);
NEED_STRING(stack_mode);

/*
 * Take an ici struct and turn it into an XWindowChanges structure.
 * Returns the value_mask representing the values defined in the
 * XWindowChanges structure. Zero means no values were set.
 */
static unsigned int
struct_to_xwindowchanges(struct_t *s, XWindowChanges *changes)
{
    object_t		*o;
    char		*what;
    unsigned int	value_mask = 0;
    static char		name[32];

    if ((o = fetch(s, STRING(x))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "x";
	    goto fail;
	}
	changes->x = intof(o)->i_value;
	value_mask |= CWX;
    }
    if ((o = fetch(s, STRING(y))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "y";
	    goto fail;
	}
	changes->y = intof(o)->i_value;
	value_mask |= CWY;
    }
    if ((o = fetch(s, STRING(width))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "y";
	    goto fail;
	}
	changes->width = intof(o)->i_value;
	value_mask |= CWWidth;
    }
    if ((o = fetch(s, STRING(height))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "height";
	    goto fail;
	}
	changes->height = intof(o)->i_value;
	value_mask |= CWHeight;
    }
    if ((o = fetch(s, STRING(border_width))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "y";
	    goto fail;
	}
	changes->border_width = intof(o)->i_value;
	value_mask |= CWBorderWidth;
    }
    if ((o = fetch(s, STRING(sibling))) != objof(&o_null))
    {
	if (!iswindow(o))
	{
	    what = "sibling";
	    goto fail;
	}
	changes->sibling = windowof(o)->w_window;
	value_mask |= CWSibling;
    }
    if ((o = fetch(s, STRING(stack_mode))) != objof(&o_null))
    {
	if (!isint(o))
	{
	    what = "stack_mode";
	    goto fail;
	}
	changes->stack_mode = intof(o)->i_value;
	value_mask |= CWStackMode;
    }
    return value_mask;

fail:
    sprintf(buf, "``%s'' incorrectly supplied as %s", what, objname(name, o));
    error = buf;
    return 1;
}

FUNC(ConfigureWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    struct_t		*s;
    XWindowChanges	changes;
    unsigned int	mask;

    if (ici_typecheck("ood", &display, &window, &s))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((mask = struct_to_xwindowchanges(s, &changes)) != 0)
	XConfigureWindow(display->d_display, window->w_window, mask, &changes);
    return null_ret();
}

FUNC(ClearWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XClearWindow(display->d_display, window->w_window);
    return null_ret();
}

FUNC(MoveWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		x, y;

    if (ici_typecheck("ooii", &display, &window, &x, &y))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XMoveWindow(display->d_display, window->w_window, x, y);
    return null_ret();
}

FUNC(ResizeWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		x, y;

    if (ici_typecheck("ooii", &display, &window, &x, &y))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XResizeWindow(display->d_display, window->w_window, x, y);
    return null_ret();
}

FUNC(DefineCursor)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    ici_X_cursor_t	*cursor;

    if (ici_typecheck("ooo", &display, &window, &cursor))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if (!iscursor(objof(cursor)))
	return ici_argerror(2);
    XDefineCursor(display->d_display, window->w_window, cursor->c_cursor);
    return null_ret();
}

FUNC(UndefineCursor)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;

    if (ici_typecheck("oo", &display, &window))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XUndefineCursor(display->d_display, window->w_window);
    return null_ret();
}

FUNC(SetWindowBackground)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		pixel;

    if (ici_typecheck("ooi", &display, &window, &pixel))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XSetWindowBackground(display->d_display, window->w_window, pixel);
    return null_ret();
}

FUNC(SetWindowBorder)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		pixel;

    if (ici_typecheck("ooi", &display, &window, &pixel))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XSetWindowBorder(display->d_display, window->w_window, pixel);
    return null_ret();
}

FUNC(SetWindowBorderWidth)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		width;

    if (ici_typecheck("ooi", &display, &window, &width))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    XSetWindowBorderWidth(display->d_display, window->w_window, width);
    return null_ret();
}

FUNC(SetWindowBackgroundPixmap)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    object_t		*pixmap;
    Pixmap		xpixmap;

    if (ici_typecheck("ooo", &display, &window, &pixmap))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if (ispixmap(objof(pixmap)))
	xpixmap = pixmapof(pixmap)->p_pixmap;
    else if (isint(objof(pixmap)))
	xpixmap = (Pixmap)intof(pixmap)->i_value;
    else
	return ici_argerror(2);
    XSetWindowBackgroundPixmap(display->d_display, window->w_window, xpixmap);
    return null_ret();
}

NEED_STRING(background_pixmap);
NEED_STRING(background_pixel);
NEED_STRING(border_pixmap);
NEED_STRING(border_pixel);
NEED_STRING(bit_gravity);
NEED_STRING(win_gravity);
NEED_STRING(backing_store);
NEED_STRING(backing_planes);
NEED_STRING(backing_pixel);
NEED_STRING(save_under);
NEED_STRING(event_mask);
NEED_STRING(do_not_propagate_mask);
NEED_STRING(override_redirect);
NEED_STRING(colormap);
NEED_STRING(cursor);

static unsigned long
struct_to_XSetWindowAttributes(struct_t *s, XSetWindowAttributes *values)
{
    char		*what;
    unsigned long	valuemask = 0;
    object_t		*o;
    static char		name[32];

    NEED_STRINGS(0);

#define	FETCH_FIELD(NAME, MASK)\
(void)(((o = fetch(s,STRING(NAME))) != objof(&o_null)) && (valuemask|=MASK)),\
what = # NAME, o

    /*
     * This macro does the work of fetching an integer value for
     * a specific GC field. Give the C field name and the mask
     * used to set valuemask.
     */
#define	FETCH_INT_FIELD(NAME, MASK)\
    do\
	if ((o = fetch(s, STRING(NAME))) != objof(&o_null))\
	{\
	    if (!isint(o))\
	    {\
		what = # NAME;\
		goto fail;\
	    }\
	    values->NAME = intof(o)->i_value;\
	    valuemask |= MASK;\
	}\
    while (0)

#define	FETCH_BOUNDED_INT_FIELD(NAME, MASK, LOWER, UPPER)\
    do\
	if ((o = fetch(s, STRING(NAME))) != objof(&o_null))\
	{\
	    if (!isint(o))\
	    {\
		what = # NAME;\
		goto fail;\
	    }\
	    if (intof(o)->i_value < LOWER || intof(o)->i_value > UPPER)\
	    {\
		error = "invalid value for " #NAME;\
		goto failed;\
	    }\
	    values->NAME = intof(o)->i_value;\
	    valuemask |= MASK;\
	}\
    while (0)

    FETCH_INT_FIELD(background_pixel, CWBackPixel);
    FETCH_INT_FIELD(border_pixel, CWBorderPixel);
    FETCH_BOUNDED_INT_FIELD(bit_gravity, CWBitGravity, ForgetGravity, StaticGravity);
    FETCH_BOUNDED_INT_FIELD(win_gravity, CWWinGravity, UnmapGravity, StaticGravity);
    FETCH_BOUNDED_INT_FIELD(backing_store, CWBackingStore, NotUseful, Always);
    FETCH_INT_FIELD(backing_planes, CWBackingPlanes);
    FETCH_INT_FIELD(backing_pixel, CWBackingPixel);
    FETCH_INT_FIELD(save_under, CWSaveUnder);
    FETCH_INT_FIELD(override_redirect, CWOverrideRedirect);
    FETCH_INT_FIELD(event_mask, CWEventMask);
    FETCH_INT_FIELD(do_not_propagate_mask, CWDontPropagate);
    if (FETCH_FIELD(background_pixmap, CWBackPixmap) != NULL)
    {
	if (!ispixmap(o))
	    goto fail;
	values->background_pixmap = pixmapof(o)->p_pixmap;
    }
    if (FETCH_FIELD(border_pixmap, CWBorderPixmap) != NULL)
    {
	if (!ispixmap(o))
	    goto fail;
	values->border_pixmap = pixmapof(o)->p_pixmap;
    }
    if (FETCH_FIELD(colormap, CWColormap) != NULL)
    {
	if (!iscolormap(o))
	    goto fail;
	values->colormap = colormapof(o)->c_colormap;
    }
    if (FETCH_FIELD(cursor, CWCursor) != NULL)
    {
	if (!iscursor(o))
	    goto fail;
	values->cursor = cursorof(o)->c_cursor;
    }

    return valuemask;

fail:
    sprintf(buf, "``%s'' incorrectly supplied as %s", what, objname(name, o));
    error = buf;

failed:
    return 0;
}

FUNC(ChangeWindowAttributes)
{
    ici_X_display_t		*display;
    ici_X_window_t		*window;
    struct_t			*attr;
    XSetWindowAttributes	xattr;
    unsigned long		attrmask;


    if (ici_typecheck("ood", &display, &window, &attr))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((attrmask = struct_to_XSetWindowAttributes(attr, &xattr)) != 0)
	XChangeWindowAttributes
	(
	    display->d_display,
	    window->w_window,
	    attrmask,
	    &xattr
	);
    return null_ret();
}

FUNC(ReparentWindow)
{
    ici_X_display_t	*display;
    ici_X_window_t	*w1; 
    ici_X_window_t	*w2;
    long		x, y;

    if (NARGS() == 3)
    {
	if (ici_typecheck("ooo", &display, &w1, &w2))
	    return 1;
	x = y = 0;
    }
    else
    {
	if (ici_typecheck("oooii", &display, &w1, &w2, &x, &y))
	    return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(w1)))
	return ici_argerror(1);
    if (!iswindow(objof(w2)))
	return ici_argerror(2);
    XReparentWindow(display->d_display, w1->w_window, w2->w_window, x, y);
    return null_ret();
}

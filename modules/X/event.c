/*
 * $Id: event.c,v 1.3 2000/07/14 08:16:01 atrn Exp $
 *
 */

#include "ici.h"
#include "null.h"
#include "event.h"
#include "display.h"
#include "window.h"
#include "atom.h"

static unsigned long
mark_event(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_event_t) + mark(eventof(o)->e_struct) + sizeof (XEvent);
}

static void
free_event(object_t *o)
{
    ici_free(eventof(o)->e_event);
    ici_free(o);
}

static object_t *
fetch_event(object_t *o, object_t *k)
{
    return fetch(eventof(o)->e_struct, k);
}

type_t	ici_X_event_type =
{
    mark_event,
    free_event,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_event,
    "event"
};

NEED_STRING(type);
NEED_STRING(serial);
NEED_STRING(send_event);
NEED_STRING(display);
NEED_STRING(window);
NEED_STRING(subwindow);
NEED_STRING(time);
NEED_STRING(x);
NEED_STRING(y);
NEED_STRING(root);
NEED_STRING(x_root);
NEED_STRING(y_root);
NEED_STRING(state);
NEED_STRING(keycode);
NEED_STRING(same_screen);
NEED_STRING(format);
NEED_STRING(message_type);
NEED_STRING(data);
NEED_STRING(button);
NEED_STRING(width);
NEED_STRING(height);
NEED_STRING(border_width);
NEED_STRING(override_redirect);
NEED_STRING(above);
NEED_STRING(detail);
NEED_STRING(value_mask);
NEED_STRING(parent);
NEED_STRING(count);
NEED_STRING(is_hint);
NEED_STRING(event);
NEED_STRING(from_configure);

NEED_STRING(KeyPress);
NEED_STRING(KeyRelease);
NEED_STRING(ButtonPress);
NEED_STRING(ButtonRelease);
NEED_STRING(EnterWindow);
NEED_STRING(LeaveWindow);
NEED_STRING(PointerMotion);
NEED_STRING(PointerMotionHint);
NEED_STRING(Button1Motion);
NEED_STRING(Button2Motion);
NEED_STRING(Button3Motion);
NEED_STRING(Button4Motion);
NEED_STRING(Button5Motion);
NEED_STRING(ButtonMotion);
NEED_STRING(KeymapState);
NEED_STRING(Exposure);
NEED_STRING(VisibilityChange);
NEED_STRING(StructureNotify);
NEED_STRING(ResizeRedirect);
NEED_STRING(SubstructureNotify);
NEED_STRING(SubstructureRedirect);
NEED_STRING(FocusChange);
NEED_STRING(PropertyChange);
NEED_STRING(ColormapChange);
NEED_STRING(OwnerGrabButton);
NEED_STRING(MotionNotify);
NEED_STRING(EnterNotify);
NEED_STRING(LeaveNotify);
NEED_STRING(FocusIn);
NEED_STRING(FocusOut);
NEED_STRING(KeymapNotify);
NEED_STRING(Expose);
NEED_STRING(GraphicsExpose);
NEED_STRING(NoExpose);
NEED_STRING(VisibilityNotify);
NEED_STRING(CreateNotify);
NEED_STRING(DestroyNotify);
NEED_STRING(UnmapNotify);
NEED_STRING(MapNotify);
NEED_STRING(MapRequest);
NEED_STRING(ReparentNotify);
NEED_STRING(ConfigureNotify);
NEED_STRING(ConfigureRequest);
NEED_STRING(GravityNotify);
NEED_STRING(ResizeRequest);
NEED_STRING(CirculateNotify);
NEED_STRING(CirculateRequest);
NEED_STRING(PropertyNotify);
NEED_STRING(SelectionClear);
NEED_STRING(SelectionRequest);
NEED_STRING(SelectionNotify);
NEED_STRING(ColormapNotify);
NEED_STRING(ClientMessage);
NEED_STRING(MappingNotify);

static array_t *
xclient_data_to_array(XEvent *e)
{
    array_t	*a;
    int_t	*v;

    if ((a = new_array()) != NULL)
    {
	int	i;

	if (pushcheck(a, 5))
	    goto fail;
	for (i = 0; i < 5; ++i)
	{
	    if ((v = new_int(e->xclient.data.l[i])) == NULL)
		goto fail;
	    *a->a_top++ = objof(v);
	    decref(v);
	}
    }
    return a;

fail:
    decref(a);
    return NULL;
}

struct_t *
ici_X_event_to_struct(XEvent *e)
{
    static char	*event_name[] =
    {
	NULL,
	NULL,
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify"
    };

    struct_t		*s;
    object_t		*v;
    ici_X_display_t	*d;

    NEED_STRINGS(NULL);

    if ((s = new_struct()) == NULL)
	return NULL;

    /* Do all common cases */
#define	DO(x,y)\
    if ((v = objof(y)) == NULL)\
	goto fail;\
    if (assign(s, STRING(x), v))\
    {\
	decref(v);\
	goto fail;\
    }\
    decref(v)

    DO(type,		new_cname(event_name[e->type]));
    DO(serial,		new_int(e->xany.serial));
    DO(send_event,	new_int(e->xany.send_event));
    d = ici_X_new_display(e->xany.display);
    DO(display,		d);
    DO(window,		ici_X_new_window(e->xany.window));

    switch (e->type)
    {
    case KeyPress:
    case KeyRelease:
	DO(root,	ici_X_new_window(e->xkey.root));
	DO(subwindow,	ici_X_new_window(e->xkey.subwindow));
	DO(time,	new_int(e->xkey.time));
	DO(x,		new_int(e->xkey.x));
	DO(y,		new_int(e->xkey.y));
	DO(x_root,	new_int(e->xkey.x_root));
	DO(y_root,	new_int(e->xkey.y_root));
	DO(state,	new_int(e->xkey.state));
	DO(keycode,	new_int(e->xkey.keycode));
	DO(same_screen,	new_int(e->xkey.same_screen));
	break;

    case ButtonPress:
    case ButtonRelease:
	DO(root,	ici_X_new_window(e->xbutton.root));
	DO(subwindow,	ici_X_new_window(e->xbutton.subwindow));
	DO(time,	new_int(e->xbutton.time));
	DO(x,		new_int(e->xbutton.x));
	DO(y,		new_int(e->xbutton.y));
	DO(x_root,	new_int(e->xbutton.x_root));
	DO(y_root,	new_int(e->xbutton.y_root));
	DO(state,	new_int(e->xbutton.state));
	DO(button,	new_int(e->xbutton.button));
	DO(same_screen,	new_int(e->xbutton.same_screen));
	break;

    case MotionNotify:
	DO(root,	ici_X_new_window(e->xmotion.root));
	DO(subwindow,	ici_X_new_window(e->xbutton.subwindow));
	DO(time,	new_int(e->xmotion.time));
	DO(x,		new_int(e->xmotion.x));
	DO(y,		new_int(e->xmotion.y));
	DO(x_root,	new_int(e->xmotion.x_root));
	DO(y_root,	new_int(e->xmotion.y_root));
	DO(state,	new_int(e->xmotion.state));
	DO(is_hint,	new_int(e->xmotion.is_hint));
	DO(same_screen,	new_int(e->xmotion.same_screen));
	break;

    case EnterNotify:
	break;

    case LeaveNotify:
	break;

    case FocusIn:
	break;

    case FocusOut:
	break;

    case KeymapNotify:
	break;

    case Expose:
	DO(x, new_int(e->xexpose.x));
	DO(y, new_int(e->xexpose.y));
	DO(width, new_int(e->xexpose.width));
	DO(height, new_int(e->xexpose.height));
	DO(count, new_int(e->xexpose.count));
	break;

    case GraphicsExpose:
	break;

    case NoExpose:
	break;

    case VisibilityNotify:
	break;

    case CreateNotify:
	DO(x,		new_int(e->xcreatewindow.x));
	DO(y,		new_int(e->xcreatewindow.y));
	DO(width,	new_int(e->xcreatewindow.width));
	DO(height,	new_int(e->xcreatewindow.height));
	DO(border_width, new_int(e->xcreatewindow.border_width));
	DO(override_redirect, new_int(e->xcreatewindow.override_redirect));
	break;

    case DestroyNotify:
	DO(event,	ici_X_new_window(e->xdestroywindow.event));
	DO(window,	ici_X_new_window(e->xdestroywindow.window));
	break;

    case UnmapNotify:
	DO(event,	ici_X_new_window(e->xmap.event));
	DO(window,	ici_X_new_window(e->xmap.window));
	DO(override_redirect, new_int(e->xmap.override_redirect));
	break;

    case MapNotify:
	DO(event,	ici_X_new_window(e->xunmap.event));
	DO(window,	ici_X_new_window(e->xunmap.window));
	DO(from_configure, new_int(e->xunmap.from_configure));
	break;

    case MapRequest:
	DO(window, ici_X_new_window(e->xmaprequest.window));
	DO(parent, ici_X_new_window(e->xmaprequest.parent));
	break;

    case ReparentNotify:
	break;

    case ConfigureNotify:
	DO(window, ici_X_new_window(e->xconfigure.window));
	DO(event, ici_X_new_window(e->xconfigure.event));
	DO(x,		new_int(e->xconfigure.x));
	DO(y,		new_int(e->xconfigure.y));
	DO(width,	new_int(e->xconfigure.width));
	DO(height,	new_int(e->xconfigure.height));
	DO(border_width, new_int(e->xconfigure.border_width));
	DO(above,	ici_X_new_window(e->xconfigure.above));
	DO(override_redirect,new_int(e->xconfigure.override_redirect));
	break;

    case ConfigureRequest:
	DO(parent,	ici_X_new_window(e->xconfigurerequest.parent));
	DO(window,	ici_X_new_window(e->xconfigurerequest.window));
	DO(x,		new_int(e->xconfigurerequest.x));
	DO(y,		new_int(e->xconfigurerequest.y));
	DO(width,	new_int(e->xconfigurerequest.width));
	DO(height,	new_int(e->xconfigurerequest.height));
	DO(border_width, new_int(e->xconfigurerequest.border_width));
	DO(above,	ici_X_new_window(e->xconfigurerequest.above));
	DO(detail,	new_int(e->xconfigurerequest.detail));
	DO(value_mask,	new_int(e->xconfigurerequest.value_mask));
	break;

    case GravityNotify:
	break;

    case ResizeRequest:
	break;

    case CirculateNotify:
	break;

    case CirculateRequest:
	break;

    case PropertyNotify:
	break;

    case SelectionClear:
	break;

    case SelectionRequest:
	break;

    case SelectionNotify:
	break;

    case ColormapNotify:
	break;

    case ClientMessage:
	DO(message_type,	ici_X_new_atom(e->xclient.message_type));
	DO(format,		new_int(e->xclient.format));
	DO(data,		xclient_data_to_array(e));
	break;

    case MappingNotify:
	break;

    default:
	sprintf(buf, "event code %d not implemented", e->type);
	error = buf;
	goto fail;
    }

    return s;

fail:
    decref(s);
    return NULL;
}

XEvent *
ici_X_struct_to_event(struct_t *s)
{
    static XEvent	xev;
    object_t		*o;

#define	FIELD_ERROR(x)\
    do\
    {\
	error = "event structure does not contain the key \"" # x "\"";\
	goto fail;\
    } while (0)


#define	TYPE_ERROR(N)\
    do\
    {\
	error = "value for key \"" # N "\" has incorrect type";\
	goto fail;\
    } while (0)



    if ((o = fetch(s, STRING(type))) == objof(&o_null))
	FIELD_ERROR(type);
    if ((xev.type = ici_X_event_for(stringof(o))) == 0)
    {
	error = "bad event type";
	goto fail;
    }

#define	GET(N, E, T, V)\
    if ((o = fetch(s, STRING(N))) == objof(&o_null))\
	FIELD_ERROR(N);\
    if (!(is ## T(o)))\
	TYPE_ERROR(N);\
    xev.E.N = V

#define	GET_INT(N, E)	GET(N, E, int, intof(o)->i_value)

#define	GET_WINDOW(N,E)\
    if (isnull(o = fetch(s, STRING(N))))\
	xev.E.N = None;\
    else if (isint(o) && intof(o)->i_value == 0)\
	xev.E.N = None;\
    else if (iswindow(o))\
	xev.E.N = windowof(o)->w_window;\
    else\
	TYPE_ERROR(N)

    GET_INT(serial, xany);
    GET_INT(send_event, xany);
    GET(display, xany, display, displayof(o)->d_display);
    GET(window, xany, window, windowof(o)->w_window);

    switch (xev.type)
    {
    case KeyPress:
    case KeyRelease:
	GET(root, xkey, window, windowof(o)->w_window);
	GET(subwindow, xkey, window, windowof(o)->w_window);
	GET_INT(time, xkey);
	GET_INT(x, xkey);
	GET_INT(y, xkey);
	GET_INT(x_root, xkey);
	GET_INT(y_root, xkey);
	GET_INT(state, xkey);
	GET_INT(keycode, xkey);
	GET_INT(same_screen, xkey);
	break;

    case ButtonPress:
    case ButtonRelease:
	GET(root, xbutton, window, windowof(o)->w_window);
	GET(subwindow, xbutton, window, windowof(o)->w_window);
	GET_INT(time, xbutton);
	GET_INT(x, xbutton);
	GET_INT(y, xbutton);
	GET_INT(x_root, xbutton);
	GET_INT(y_root, xbutton);
	GET_INT(state, xbutton);
	GET_INT(button, xbutton);
	GET_INT(same_screen, xbutton);
	break;

    case MotionNotify:
	break;
    case EnterNotify:
	break;
    case LeaveNotify:
	break;
    case FocusIn:
	break;
    case FocusOut:
	break;
    case KeymapNotify:
	break;

    case Expose:
	GET_INT(x, xexpose);
	GET_INT(y, xexpose);
	GET_INT(width, xexpose);
	GET_INT(height, xexpose);
	GET_INT(count, xexpose);
	break;

    case GraphicsExpose:
	break;
    case NoExpose:
	break;
    case VisibilityNotify:
	break;
    case CreateNotify:
	break;
    case DestroyNotify:
	break;
    case UnmapNotify:
	break;
    case MapNotify:
	break;
    case MapRequest:
	break;
    case ReparentNotify:
	break;

    case ConfigureNotify:
	GET_INT(x, xconfigure);
	GET_INT(y, xconfigure);
	GET_INT(width, xconfigure);
	GET_INT(height, xconfigure);
	GET_INT(border_width, xconfigure);
	GET_WINDOW(above, xconfigure);
	GET_INT(override_redirect, xconfigure);
	break;

    case ConfigureRequest:
	break;
    case GravityNotify:
	break;
    case ResizeRequest:
	break;
    case CirculateNotify:
	break;
    case CirculateRequest:
	break;
    case PropertyNotify:
	break;
    case SelectionClear:
	break;
    case SelectionRequest:
	break;
    case SelectionNotify:
	break;
    case ColormapNotify:
	break;
    case ClientMessage:
	break;
    case MappingNotify:
	break;
    default:
	goto fail;
    }
    return &xev;

fail:
    if (error == NULL)
	error = "invalid event structure";
    return NULL;
}

ici_X_event_t *
ici_X_new_event(XEvent *event)
{
    object_t	*o;

    if ((o = objof(talloc(ici_X_event_t))) != NULL)
    {
	o->o_tcode = TC_OTHER;
	o->o_flags = 0;
	o->o_nrefs = 1;
	o->o_type  = &ici_X_event_type;
	eventof(o)->e_event = event;
	eventof(o)->e_struct = ici_X_event_to_struct(event);
	if (eventof(o)->e_struct == NULL)
	    goto fail;
	rego(o);
	decref(eventof(o)->e_struct); /* Only safe to decref after rego */
    }
    return eventof(o);

fail:
    ici_free(o);
    return NULL;
}

/**
 * NextEvent - return the next event
 */
FUNC(NextEvent)
{
    XEvent		*xevent;
    ici_X_display_t	*display;

    if (ici_typecheck("o", &display))
	return 1;
    if (!isdisplay(objof(display)))
	return 1;
    if ((xevent = talloc(XEvent)) == NULL)
	return 1;
    XNextEvent(display->d_display, xevent);
    return ici_ret_with_decref(objof(ici_X_new_event(xevent)));
}

/**
 * WindowEvent - return an event for a specific window
 */
FUNC(WindowEvent)
{
    XEvent		*xevent;
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		event_mask;

    if (NARGS() == 2)
    {
	if (ici_typecheck("oo", &display, &window))
	    return 1;
	event_mask = 
		KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
		| EnterWindowMask | LeaveWindowMask
		| PointerMotionMask | PointerMotionHintMask
		| Button1MotionMask | Button2MotionMask
		| Button3MotionMask | Button4MotionMask
		| Button5MotionMask | ButtonMotionMask
		| KeymapStateMask | ExposureMask | VisibilityChangeMask
		| StructureNotifyMask | ResizeRedirectMask
		| SubstructureNotifyMask | SubstructureRedirectMask
		| FocusChangeMask | PropertyChangeMask
		| ColormapChangeMask | OwnerGrabButtonMask;
    }
    else
    {
	if (ici_typecheck("ooi", &display, &window, &event_mask))
	    return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((xevent = talloc(XEvent)) == NULL)
	return 1;
    XWindowEvent(display->d_display, window->w_window, event_mask, xevent);
    return ici_ret_with_decref(objof(ici_X_new_event(xevent)));
}


/**
 * CheckWindowEvent - return an event for a specific window
 */
FUNC(CheckWindowEvent)
{
    XEvent		*xevent;
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		event_mask;

    if (NARGS() == 2)
    {
	if (ici_typecheck("oo", &display, &window))
	    return 1;
	event_mask = 
		KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
		| EnterWindowMask | LeaveWindowMask
		| PointerMotionMask | PointerMotionHintMask
		| Button1MotionMask | Button2MotionMask
		| Button3MotionMask | Button4MotionMask
		| Button5MotionMask | ButtonMotionMask
		| KeymapStateMask | ExposureMask | VisibilityChangeMask
		| StructureNotifyMask | ResizeRedirectMask
		| SubstructureNotifyMask | SubstructureRedirectMask
		| FocusChangeMask | PropertyChangeMask
		| ColormapChangeMask | OwnerGrabButtonMask;
    }
    else
    {
	if (ici_typecheck("ooi", &display, &window, &event_mask))
	    return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iswindow(objof(window)))
	return ici_argerror(1);
    if ((xevent = talloc(XEvent)) == NULL)
	return 1;
    if (XCheckWindowEvent(display->d_display, window->w_window, event_mask, xevent))
	return ici_ret_with_decref(objof(ici_X_new_event(xevent)));
    return null_ret();
}


long
ici_X_event_mask_for(string_t *s)
{
    NEED_STRINGS(0);
    if (s == STRING(KeyPress))
	return KeyPressMask;
    if (s == STRING(KeyRelease))
	return KeyReleaseMask;
    if (s == STRING(ButtonPress))
	return ButtonPressMask;
    if (s == STRING(ButtonRelease))
	return ButtonReleaseMask;
    if (s == STRING(EnterWindow))
	return EnterWindowMask;
    if (s == STRING(LeaveWindow))
	return LeaveWindowMask;
    if (s == STRING(PointerMotion))
	return PointerMotionMask;
    if (s == STRING(PointerMotionHint))
	return PointerMotionHintMask;
    if (s == STRING(Button1Motion))
	return Button1MotionMask;
    if (s == STRING(Button2Motion))
	return Button2MotionMask;
    if (s == STRING(Button3Motion))
	return Button3MotionMask;
    if (s == STRING(Button4Motion))
	return Button4MotionMask;
    if (s == STRING(Button5Motion))
	return Button5MotionMask;
    if (s == STRING(ButtonMotion))
	return ButtonMotionMask;
    if (s == STRING(KeymapState))
	return KeymapStateMask;
    if (s == STRING(Exposure))
	return ExposureMask;
    if (s == STRING(VisibilityChange))
	return VisibilityChangeMask;
    if (s == STRING(StructureNotify))
	return StructureNotifyMask;
    if (s == STRING(ResizeRedirect))
	return ResizeRedirectMask;
    if (s == STRING(SubstructureNotify))
	return SubstructureNotifyMask;
    if (s == STRING(SubstructureRedirect))
	return SubstructureRedirectMask;
    if (s == STRING(FocusChange))
	return FocusChangeMask;
    if (s == STRING(PropertyChange))
	return PropertyChangeMask;
    if (s == STRING(ColormapChange))
	return ColormapChangeMask;
    if (s == STRING(OwnerGrabButton))
	return OwnerGrabButtonMask;
    return 0;
}

long
ici_X_event_for(string_t *s)
{
    NEED_STRINGS(0);
    if (s == STRING(KeyPress))
	return KeyPress;
    if (s == STRING(KeyRelease))
	return KeyRelease;
    if (s == STRING(ButtonPress))
	return ButtonPress;
    if (s == STRING(ButtonRelease))
	return ButtonRelease;
    if (s == STRING(MotionNotify))
	return MotionNotify;
    if (s == STRING(EnterNotify))
	return EnterNotify;
    if (s == STRING(LeaveNotify))
	return LeaveNotify;
    if (s == STRING(FocusIn))
	return FocusIn;
    if (s == STRING(FocusOut))
	return FocusOut;
    if (s == STRING(KeymapNotify))
	return KeymapNotify;
    if (s == STRING(Expose))
	return Expose;
    if (s == STRING(GraphicsExpose))
	return GraphicsExpose;
    if (s == STRING(NoExpose))
	return NoExpose;
    if (s == STRING(VisibilityNotify))
	return VisibilityNotify;
    if (s == STRING(CreateNotify))
	return CreateNotify;
    if (s == STRING(DestroyNotify))
	return DestroyNotify;
    if (s == STRING(UnmapNotify))
	return UnmapNotify;
    if (s == STRING(MapNotify))
	return MapNotify;
    if (s == STRING(MapRequest))
	return MapRequest;
    if (s == STRING(ReparentNotify))
	return ReparentNotify;
    if (s == STRING(ConfigureNotify))
	return ConfigureNotify;
    if (s == STRING(ConfigureRequest))
	return ConfigureRequest;
    if (s == STRING(GravityNotify))
	return GravityNotify;
    if (s == STRING(ResizeRequest))
	return ResizeRequest;
    if (s == STRING(CirculateNotify))
	return CirculateNotify;
    if (s == STRING(CirculateRequest))
	return CirculateRequest;
    if (s == STRING(PropertyNotify))
	return PropertyNotify;
    if (s == STRING(SelectionClear))
	return SelectionClear;
    if (s == STRING(SelectionRequest))
	return SelectionRequest;
    if (s == STRING(SelectionNotify))
	return SelectionNotify;
    if (s == STRING(ColormapNotify))
	return ColormapNotify;
    if (s == STRING(ClientMessage))
	return ClientMessage;
    if (s == STRING(MappingNotify))
	return MappingNotify;
    return 0;
}

FUNC(SendEvent)
{
    ici_X_display_t	*display;
    ici_X_window_t	*window;
    long		propogate;
    string_t		*evname;
    unsigned long	evmask;
    struct_t		*ev;
    XEvent		*xev;

    if (ici_typecheck("ooiod", &display, &window, &propogate, &evname, &ev))
	return 1;
    if (!isdisplay(objof(display)))
	return 1;
    if (!iswindow(objof(window)))
	return 1;
    if (!isstring(objof(evname)))
	return 1;
    if
    (
	(evmask = ici_X_event_mask_for(evname)) == 0
	&&
	(evmask = ici_X_event_for(evname)) == 0
    )
    {
	error = "Unknown event mask";
	return 1;
    }
    if ((xev = ici_X_struct_to_event(ev)) == NULL)
	return 1;
    XSendEvent
    (
	display->d_display,
	window->w_window,
	(Bool)propogate,
	evmask,
	xev
    );
    return null_ret();
}

FUNC(QLength)
{
    ici_X_display_t	*display;

    if (ici_typecheck("o", &display))
	return 1;
    return int_ret(QLength(display->d_display));
}

FUNC(Pending)
{
    ici_X_display_t	*display;

    if (ici_typecheck("o", &display))
	return 1;
    return int_ret(XPending(display->d_display));
}

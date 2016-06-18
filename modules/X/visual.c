/*
 * $Id: visual.c,v 1.2 2000/07/29 07:01:05 atrn Exp $
 *
 * A type to represent X11 visuals and functions that operate on them.
 */

#include "ici.h"
#include "display.h"
#include "visual.h"

NEED_STRING(visual);
NEED_STRING(visualid);
NEED_STRING(class);
NEED_STRING(depth);
NEED_STRING(red_mask);
NEED_STRING(green_mask);
NEED_STRING(blue_mask);
NEED_STRING(bits_per_rgb);
NEED_STRING(colormap_size);
NEED_STRING(map_entries);

static unsigned long
mark_visual(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_visual_t);
}

static object_t *
fetch_visual(object_t *o, object_t *k)
{
    NEED_STRINGS(NULL);
    if (k == objof(STRING(visualid)))
	return objof(new_int(visualof(o)->v_visual->visualid));
    if (k == objof(STRING(class)))
	return objof(new_int(visualof(o)->v_visual->class));
    if (k == objof(STRING(red_mask)))
	return objof(new_int(visualof(o)->v_visual->red_mask));
    if (k == objof(STRING(green_mask)))
	return objof(new_int(visualof(o)->v_visual->green_mask));
    if (k == objof(STRING(blue_mask)))
	return objof(new_int(visualof(o)->v_visual->blue_mask));
    if (k == objof(STRING(bits_per_rgb)))
	return objof(new_int(visualof(o)->v_visual->bits_per_rgb));
    if (k == objof(STRING(map_entries)))
	return objof(new_int(visualof(o)->v_visual->map_entries));
    return objof(&o_null);
}

type_t	ici_X_visual_type =
{
    mark_visual,
    free_simple,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_visual,
    "visual"
};

ici_X_visual_t *
ici_X_new_visual(Visual *vis)
{
    ici_X_visual_t	*v;

    if ((v = talloc(ici_X_visual_t)) != NULL)
    {
	objof(v)->o_type  = &ici_X_visual_type;
	objof(v)->o_tcode = TC_OTHER;
	objof(v)->o_flags = 0;
	objof(v)->o_nrefs = 1;
	rego(objof(v));
	v->v_visual = vis;
    }
    return v;
}

FUNC(DefaultVisual)
{
    ici_X_display_t	*display;
    long		screen;
    Visual		*vis;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    vis = DefaultVisual(display->d_display, screen);
    return ici_ret_with_decref(objof(ici_X_new_visual(vis)));
}

FUNC(VisualIDFromVisual)
{
    ici_X_visual_t	*v;

    if (ici_typecheck("o", &v))
	return 1;
    if (!isvisual(objof(v)))
	return ici_argerror(0);
    return int_ret(XVisualIDFromVisual(v->v_visual));
}

struct_t *
ici_X_visualinfo_to_struct(XVisualInfo *vi)
{
    struct_t		*s;
    int_t		*i;
    ici_X_visual_t	*v;

    NEED_STRINGS(NULL);
    if ((s = new_struct()) != NULL)
    {
	if ((v = ici_X_new_visual(vi->visual)) == NULL)
	    goto fail;
	if (assign(s, STRING(visual), v))
	    goto fail;
	decref(v);
    }

#define	FIELD(NAME)\
    if ((i = new_int(vi->NAME)) == NULL)\
	goto fail;\
    if (assign(s, STRING(NAME), i))\
	goto fail;\
    decref(i)

    FIELD(visualid);
    FIELD(screen);
    FIELD(class);
    FIELD(red_mask);
    FIELD(green_mask);
    FIELD(blue_mask);
    FIELD(colormap_size);
    FIELD(bits_per_rgb);

#undef FIELD

    return s;

fail:
    decref(s);
    return NULL;
}


long
ici_X_struct_to_visualinfo(struct_t *s, XVisualInfo *vi)
{
    long	mask = 0;
    object_t	*o;

    NEED_STRINGS(0);
#define	FIELD(NAME, MASK)\
    do\
	if ((o = fetch(s, STRING(NAME))) != objof(&o_null))\
	{\
	    mask |= MASK;\
	    vi->NAME = intof(o)->i_value;\
	}\
    while (0)

    FIELD(visualid, VisualIDMask);
    FIELD(screen, VisualScreenMask);
    FIELD(class, VisualClassMask);
    FIELD(red_mask, VisualRedMaskMask);
    FIELD(green_mask, VisualGreenMaskMask);
    FIELD(blue_mask, VisualBlueMaskMask);
    FIELD(colormap_size, VisualColormapSizeMask);
    FIELD(bits_per_rgb, VisualBitsPerRGBMask);

#undef FIELD

    return mask;
}

FUNC(GetVisualInfo)
{
    ici_X_display_t	*display;
    XVisualInfo		visinfo;
    long		visinfomask;
    int			i, n;
    XVisualInfo		*v;
    array_t		*a;
    struct_t		*s;

    if (ici_typecheck("od", &display, &s))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if ((visinfomask = ici_X_struct_to_visualinfo(s, &visinfo)) == 0)
    {
	error = "no fields in visual template";
	return 1;
    }
    v = XGetVisualInfo(display->d_display, visinfomask, &visinfo, &n);
    if (n == 0)
    {
	error = "no matching visuals";
	return 1;
    }
    if ((a = new_array()) == NULL)
	return 1;
    for (i = 0; i < n; ++i)
    {
	if ((s = ici_X_visualinfo_to_struct(v)) == NULL)
	{
	    decref(a);
	    return 1;
	}
	if (pushcheck(a, 1))
	{
	    decref(s);
	    decref(a);
	    return 1;
	}
	*a->a_top++ = objof(s);
	decref(s);
    }
    return ici_ret_with_decref(objof(a));
}

FUNC(MatchVisualInfo)
{
    ici_X_display_t	*display;
    long		screen;
    long		depth;
    long		class;
    struct_t		*s;
    XVisualInfo		vi;

    if (ici_typecheck("oiii", &display, &screen, &depth, &class))
	return 1;
    if (!isdisplay(objof(display)))
    return ici_argerror(0);
    if (!XMatchVisualInfo(display->d_display, screen, depth, class, &vi))
    {
	error = "no matching visual";
	return 1;
    }
    if ((s = ici_X_visualinfo_to_struct(&vi)) == NULL)
	return 1;
    return ici_ret_with_decref(objof(s));
}

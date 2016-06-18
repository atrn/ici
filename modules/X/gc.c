/*
 * $Id: gc.c,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#include "ici.h"
#include "gc.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "font.h"

static unsigned long
mark_gc(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_gc_t);
}

static unsigned long
hash_gc(object_t *o)
{
    return (unsigned long)gcof(o)->g_gc * UNIQUE_PRIME;
}

static int
cmp_gc(object_t *a, object_t *b)
{
    return gcof(a)->g_gc != gcof(b)->g_gc;
}

type_t	ici_X_gc_type =
{
    mark_gc,
    free_simple,
    hash_gc,
    cmp_gc,
    copy_simple,
    assign_simple,
    fetch_simple,
    "GC"
};

INLINE
static object_t *
atom_gc(GC gc)
{
    object_t	**po, *o;

    for
    (
	po = &atoms[ici_atom_hash_index((unsigned long)gc * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if (isgc(o) && gcof(gc)->g_gc == gc)
	    return o;
    }
    return NULL;
}

ici_X_gc_t *
ici_X_new_gc(GC gc)
{
    ici_X_gc_t	*g;

    if ((g = gcof(atom_gc(gc))) != NULL)
    {
	incref(objof(g));
	return g;
    }
    if ((g = talloc(ici_X_gc_t)) != NULL)
    {
	objof(g)->o_type  = &ici_X_gc_type;
	objof(g)->o_tcode = TC_OTHER;
	objof(g)->o_flags = 0;
	objof(g)->o_nrefs = 1;
	rego(objof(g));
	g->g_gc = gc;
    }
    return gcof(atom(objof(g), 1));
}

NEED_STRING(function);
NEED_STRING(plane_mask);
NEED_STRING(foreground);
NEED_STRING(background);
NEED_STRING(line_width);
NEED_STRING(line_style);
NEED_STRING(cap_style);
NEED_STRING(join_style);
NEED_STRING(fill_style);
NEED_STRING(fill_rule);
NEED_STRING(arc_mode);
NEED_STRING(tile);
NEED_STRING(stipple);
NEED_STRING(ts_x_origin);
NEED_STRING(ts_y_origin);
NEED_STRING(font);
NEED_STRING(subwindow_mode);
NEED_STRING(graphics_exposures);
NEED_STRING(clip_x_origin);
NEED_STRING(clip_y_origin);
NEED_STRING(clip_mask);
NEED_STRING(dash_offset);
NEED_STRING(dash_list);

/*
 * Map an ici struct form of an XGCValues into a real XGCValues and
 * return the ``valuemask'' for the XGCValues structure.
 */
static unsigned long
struct_to_XGCValues(struct_t *s, XGCValues *values)
{
    char		*what;
    object_t		*o;
    unsigned long	valuemask = 0;
    static char		name[32];

    NEED_STRINGS(0);

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

    /*
     * Fetch fields...
     */

    FETCH_BOUNDED_INT_FIELD(function, GCFunction,
	GXclear, GXset);
    FETCH_INT_FIELD(plane_mask, GCPlaneMask);
    FETCH_INT_FIELD(foreground, GCForeground);
    FETCH_INT_FIELD(background, GCBackground);
    FETCH_INT_FIELD(line_width, GCLineWidth);
    FETCH_BOUNDED_INT_FIELD(line_style, GCLineStyle,
	LineSolid, LineDoubleDash);
    FETCH_BOUNDED_INT_FIELD(cap_style, GCCapStyle,
	CapNotLast, CapProjecting);
    FETCH_BOUNDED_INT_FIELD(join_style, GCJoinStyle,
	JoinMiter, JoinBevel);
    FETCH_BOUNDED_INT_FIELD(fill_style, GCFillStyle,
	FillSolid, FillOpaqueStippled);
    FETCH_BOUNDED_INT_FIELD(fill_rule, GCFillRule,
	EvenOddRule, WindingRule);
    FETCH_BOUNDED_INT_FIELD(arc_mode, GCArcMode,
	ArcChord, ArcPieSlice);
    /* tile - Pixmap */
    /* stipple - Pixmap */
    FETCH_INT_FIELD(ts_x_origin, GCTileStipXOrigin);
    FETCH_INT_FIELD(ts_y_origin, GCTileStipYOrigin);
    /* font - Font */
    FETCH_BOUNDED_INT_FIELD(subwindow_mode, GCSubwindowMode,
	ClipByChildren, IncludeInferiors);
    FETCH_INT_FIELD(graphics_exposures, GCGraphicsExposures);
    FETCH_INT_FIELD(clip_x_origin, GCClipXOrigin);
    FETCH_INT_FIELD(clip_y_origin, GCClipYOrigin);
    /* clip_mask - Pixmap */
    FETCH_INT_FIELD(dash_offset, GCDashOffset);
    /* dashes */
    return valuemask;

fail:
    sprintf(buf, "``%s'' incorrectly supplied as %s", what, objname(name, o));
    error = buf;

failed:
    return ~0;
}

static struct_t *
XGCValues_to_struct(XGCValues *xgcvalues, unsigned long valuemask)
{
    struct_t	*s;
    int_t	*i;

    NEED_STRINGS(NULL);

    if ((s = new_struct()) == NULL)
	return NULL;

#define	SET_INT_FIELD(NAME, MASK)\
	do\
	    if (valuemask & MASK)\
	    {\
		if ((i = new_int(xgcvalues->NAME)) == NULL)\
		    goto fail;\
		if (assign(s, STRING(NAME), i))\
		{\
		    decref(i);\
		    goto fail;\
		}\
		decref(i);\
	    }\
	while (0)

    SET_INT_FIELD(function, GCFunction);
    SET_INT_FIELD(plane_mask, GCPlaneMask);
    SET_INT_FIELD(foreground, GCForeground);
    SET_INT_FIELD(background, GCBackground);
    SET_INT_FIELD(line_width, GCLineWidth);
    SET_INT_FIELD(line_style, GCLineStyle);
    SET_INT_FIELD(cap_style, GCCapStyle);
    SET_INT_FIELD(join_style, GCJoinStyle);
    SET_INT_FIELD(fill_style, GCFillStyle);
    SET_INT_FIELD(fill_rule, GCFillRule);
    SET_INT_FIELD(arc_mode, GCArcMode);
    /* tile - Pixmap */
    /* stipple - Pixmap */
    SET_INT_FIELD(ts_x_origin, GCTileStipXOrigin);
    SET_INT_FIELD(ts_y_origin, GCTileStipYOrigin);
    /* font - Font */
    SET_INT_FIELD(subwindow_mode, GCSubwindowMode);
    SET_INT_FIELD(graphics_exposures, GCGraphicsExposures);
    SET_INT_FIELD(clip_x_origin, GCClipXOrigin);
    SET_INT_FIELD(clip_y_origin, GCClipYOrigin);
    /* clip_mask - Pixmap */
    SET_INT_FIELD(dash_offset, GCDashOffset);
    /* dashes */

    return s;

fail:
    decref(s);
    return NULL;
}


/**
 * CreateGC
 */
FUNC(CreateGC)
{
    ici_X_display_t	*display;
    object_t		*drawable;	/* window or pixmap */
    struct_t		*values;
    Drawable		d;
    XGCValues		gcvalues;
    GC			gc;
    unsigned long	valuemask;

    if (ici_typecheck("ood", &display, &drawable, &values))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
        d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if ((valuemask = struct_to_XGCValues(values, &gcvalues)) == ~0)
	return 1;
    gc = XCreateGC(display->d_display, d, valuemask, &gcvalues);
    return ici_ret_with_decref(atom(objof(ici_X_new_gc(gc)), 1));
}

FUNC(CopyGC)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*src;
    ici_X_gc_t		*dst;
    long		valuemask;

    if (ici_typecheck("oooi", &display, &src, &dst, &valuemask))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(src)))
	return ici_argerror(1);
    if (!isgc(objof(dst)))
	return ici_argerror(2);
    XCopyGC(display->d_display, src->g_gc, valuemask, dst->g_gc);
    return null_ret();
}

FUNC(ChangeGC)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    struct_t		*gcvalues;
    XGCValues		xgcvalues;
    unsigned long	valuemask;

    if (ici_typecheck("ood", &display, &gc, &gcvalues))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    valuemask = struct_to_XGCValues(gcvalues, &xgcvalues);
    XChangeGC(display->d_display, gc->g_gc, valuemask, &xgcvalues);
    return null_ret();
}

FUNC(GetGCValues)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		valuemask;
    XGCValues		xgcvalues;
    struct_t		*values;

    if (ici_typecheck("ooi", &display, &gc, &valuemask))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    XGetGCValues(display->d_display, gc->g_gc, valuemask, &xgcvalues);
    if ((values = XGCValues_to_struct(&xgcvalues, valuemask)) == NULL)
	return 1;
    return ici_ret_with_decref(objof(values));
}

FUNC(FreeGC)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;

    if (ici_typecheck("oo", &display, &gc))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    XFreeGC(display->d_display, gc->g_gc);
    return null_ret();
}

FUNC(SetFillRule)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		fillrule;

    if (ici_typecheck("ooi", &display, &gc, &fillrule))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (fillrule < EvenOddRule || fillrule > WindingRule)
	return ici_argerror(2);
    XSetFillRule(display->d_display, gc->g_gc, fillrule);
    return null_ret();
}

FUNC(SetFillStyle)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		fillstyle;

    if (ici_typecheck("ooi", &display, &gc, &fillstyle))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (fillstyle < FillSolid || fillstyle > FillOpaqueStippled)
	return ici_argerror(2);
    XSetFillStyle(display->d_display, gc->g_gc, fillstyle);
    return null_ret();
}

FUNC(SetLineAttributes)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		line_width;
    long		line_style;
    long		cap_style;
    long		join_style;

    if (ici_typecheck("ooiiii", &display, &gc, &line_width, &line_style, &cap_style, &join_style))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (line_width < 0)
	return ici_argerror(2);
    if (line_style < LineSolid || line_style > LineDoubleDash)
	return ici_argerror(3);
    if (cap_style < CapNotLast || cap_style > CapProjecting)
	return ici_argerror(4);
    if (join_style < JoinMiter || join_style > JoinBevel)
	return ici_argerror(5);
    XSetLineAttributes(display->d_display, gc->g_gc, line_width, line_style, cap_style, join_style);
    return null_ret();
}

FUNC(SetState)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		fg, bg;
    long		function;
    long		plane_mask;

    if (ici_typecheck("ooiiii", &display, &gc, &fg, &bg, &function, &plane_mask))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (function < GXclear || function > GXset)
	return ici_argerror(4);
    XSetState(display->d_display, gc->g_gc, fg, bg, function, plane_mask);
    return null_ret();
}

FUNC(SetFunction)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		function;

    if (ici_typecheck("ooi", &display, &gc, &function))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (function < GXclear || function > GXset)
	return ici_argerror(2);
    XSetFunction(display->d_display, gc->g_gc, function);
    return null_ret();
}

FUNC(SetPlaneMask)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		plane_mask;

    if (ici_typecheck("ooi", &display, &gc, &plane_mask))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    XSetPlaneMask(display->d_display, gc->g_gc, plane_mask);
    return null_ret();
}

FUNC(SetForeground)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		color;

    if (ici_typecheck("ooi", &display, &gc, &color))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    XSetForeground(display->d_display, gc->g_gc, color);
    return null_ret();
}

FUNC(SetBackground)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    long		color;

    if (ici_typecheck("ooi", &display, &gc, &color))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    XSetBackground(display->d_display, gc->g_gc, color);
    return null_ret();
}

FUNC(SetFont)
{
    ici_X_display_t	*display;
    ici_X_gc_t		*gc;
    ici_X_font_t	*font;

    if (ici_typecheck("ooo", &display, &gc, &font))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isgc(objof(gc)))
	return ici_argerror(1);
    if (!isfont(objof(font)))
	return ici_argerror(2);
    XSetFont(display->d_display, gc->g_gc, font->f_font);
    return null_ret();
}

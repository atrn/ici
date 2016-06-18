/*
 * $Id: graphics.c,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#include "ici.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "gc.h"

NEED_STRING(x1);
NEED_STRING(x2);
NEED_STRING(y1);
NEED_STRING(y2);

FUNC(DrawPoint)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y;

    if (ici_typecheck("oooii", &display, &drawable, &gc, &x, &y))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XDrawPoint(display->d_display, d, gc->g_gc, x, y);
    return null_ret();
}

FUNC(DrawPoints)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*points;
    int			npoints;
    long		mode;
    XPoint		*xpoints;
    XPoint		*xp;
    object_t		**po;
    int_t		*x, *y;

    NEED_STRINGS(1);

    if (ici_typecheck("oooai", &display, &drawable, &gc, &points, &mode))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    npoints = points->a_top - points->a_base;
    if ((xpoints = ici_alloc(sizeof (XPoint) * npoints)) == NULL)
	return 1;
    for (po = points->a_base, xp = xpoints; xp - xpoints < npoints; ++xp, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	)
	{
	    error = "invalid point in point array";
	    ici_free(xpoints);
	    return 1;
	}
	xp->x = x->i_value;
	xp->y = y->i_value;
    }
    XDrawPoints(display->d_display, d, gc->g_gc, xpoints, npoints, mode);
    ici_free(xpoints);
    return null_ret();
}

FUNC(DrawLine)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x1, y1, x2, y2;

    if (ici_typecheck("oooiiii", &display, &drawable, &gc, &x1, &y1, &x2, &y2))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XDrawLine(display->d_display, d, gc->g_gc, x1, y1, x2, y2);
    return null_ret();
}

FUNC(DrawLines)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*points;
    int			npoints;
    long		mode;
    XPoint		*xpoints;
    XPoint		*xp;
    object_t		**po;
    int_t		*x, *y;

    NEED_STRINGS(1);

    if (ici_typecheck("oooai", &display, &drawable, &gc, &points, &mode))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    npoints = points->a_top - points->a_base;
    if ((xpoints = ici_alloc(sizeof (XPoint) * npoints)) == NULL)
	return 1;
    for (po = points->a_base, xp = xpoints; xp - xpoints < npoints; ++xp, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	)
	{
	    error = "invalid point in point array";
	    ici_free(xpoints);
	    return 1;
	}
	xp->x = x->i_value;
	xp->y = y->i_value;
    }
    XDrawLines(display->d_display, d, gc->g_gc, xpoints, npoints, mode);
    ici_free(xpoints);
    return null_ret();
}

FUNC(DrawSegments)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*segments;
    int			nsegments;
    XSegment		*xsegments;
    XSegment		*xseg;
    object_t		**po;
    int_t		*x1, *y1, *x2, *y2;

    NEED_STRINGS(1);

    if (ici_typecheck("oooa", &display, &drawable, &gc, &segments))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    nsegments = segments->a_top - segments->a_base;
    if ((xsegments = ici_alloc(sizeof (XSegment) * nsegments)) == NULL)
	return 1;
    for (po = segments->a_base, xseg = xsegments; xseg - xsegments < nsegments; ++xseg, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x1 = intof(fetch(*po, STRING(x1)))) == objof(&o_null)
	    ||
	    !isint(objof(x1))
	    ||
	    objof(y1 = intof(fetch(*po, STRING(y1)))) == objof(&o_null)
	    ||
	    !isint(objof(y1))
	    ||
	    objof(x2 = intof(fetch(*po, STRING(x2)))) == objof(&o_null)
	    ||
	    !isint(objof(x2))
	    ||
	    objof(y2 = intof(fetch(*po, STRING(y2)))) == objof(&o_null)
	    ||
	    !isint(objof(y2))
	)
	{
	    error = "invalid segment in segment array";
	    ici_free(xsegments);
	    return 1;
	}
	xseg->x1 = x1->i_value;
	xseg->y1 = y1->i_value;
	xseg->x2 = x2->i_value;
	xseg->y2 = y2->i_value;
    }
    XDrawSegments(display->d_display, d, gc->g_gc, xsegments, nsegments);
    ici_free(xsegments);
    return null_ret();
}

FUNC(DrawRectangle)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y, w, h;

    if (ici_typecheck("oooiiii", &display, &drawable, &gc, &x, &y, &w, &h))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XDrawRectangle(display->d_display, d, gc->g_gc, x, y, w, h);
    return null_ret();
}

FUNC(DrawRectangles)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*rects;
    object_t		**po;
    XRectangle		*xrects;
    XRectangle		*xr;
    int			nrects;
    int_t		*x, *y, *width, *height;

    NEED_STRINGS(1);

    if (ici_typecheck("oooa", &display, &drawable, &gc, &rects))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    nrects = rects->a_top - rects->a_base;
    if ((xrects = ici_alloc(sizeof (XRectangle) * nrects)) == NULL)
	return 1;
    for (po = rects->a_base, xr = xrects; xr - xrects < nrects; ++xr, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	    ||
	    objof(width = intof(fetch(*po, STRING(width)))) == objof(&o_null)
	    ||
	    !isint(objof(width))
	    ||
	    objof(height = intof(fetch(*po, STRING(height)))) == objof(&o_null)
	    ||
	    !isint(objof(height))
	)
	{
	    error = "invalid rectangle in rectangle array";
	    ici_free(xrects);
	    return 1;
	}
	xr->x = x->i_value;
	xr->y = y->i_value;
	xr->width = width->i_value;
	xr->height = height->i_value;
    }
    XDrawRectangles(display->d_display, d, gc->g_gc, xrects, nrects);
    ici_free(xrects);
    return null_ret();
}

FUNC(DrawArc)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y;
    long		width, height;
    long		angle1, angle2;

    if (ici_typecheck("oooiiiiii", &display, &drawable, &gc,
			&x, &y, &width, &height, &angle1, &angle2))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XDrawArc(display->d_display, d, gc->g_gc, x, y, width, height, angle1, angle2);
    return null_ret();
}

NEED_STRING(angle1);
NEED_STRING(angle2);

FUNC(DrawArcs)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*arcs;
    object_t		**po;
    int			narcs;
    XArc		*xarcs;
    XArc		*xa;
    int_t		*x, *y, *width, *height, *a1, *a2;

    NEED_STRINGS(1);

    if (ici_typecheck("oooa", &display, &drawable, &gc, &arcs))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    narcs = arcs->a_top - arcs->a_base;
    if ((xarcs = ici_alloc(sizeof (XArc) * narcs)) == NULL)
	return 1;
    for (po = arcs->a_base, xa = xarcs; xa - xarcs < narcs; ++xa, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	    ||
	    objof(width = intof(fetch(*po, STRING(width)))) == objof(&o_null)
	    ||
	    !isint(objof(width))
	    ||
	    objof(height = intof(fetch(*po, STRING(height)))) == objof(&o_null)
	    ||
	    !isint(objof(height))
	    ||
	    objof(a1 = intof(fetch(*po, STRING(angle1)))) == objof(&o_null)
	    ||
	    !isint(objof(a1))
	    ||
	    objof(a2 = intof(fetch(*po, STRING(angle2)))) == objof(&o_null)
	    ||
	    !isint(objof(a2))
	)
	{
	    error = "invalid arc in arc array";
	    ici_free(xarcs);
	    return 1;
	}
	xa->x = x->i_value;
	xa->y = y->i_value;
	xa->width = width->i_value;
	xa->height = height->i_value;
	xa->angle1 = a1->i_value;
	xa->angle2 = a2->i_value;
    }
    XDrawArcs(display->d_display, d, gc->g_gc, xarcs, narcs);
    ici_free(xarcs);
    return null_ret();
}

FUNC(FillRectangle)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y, w, h;

    if (ici_typecheck("oooiiii", &display, &drawable, &gc, &x, &y, &w, &h))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XFillRectangle(display->d_display, d, gc->g_gc, x, y, w, h);
    return null_ret();
}

FUNC(FillRectangles)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*rects;
    object_t		**po;
    XRectangle		*xrects;
    XRectangle		*xr;
    int			nrects;
    int_t		*x, *y, *width, *height;

    NEED_STRINGS(1);

    if (ici_typecheck("oooa", &display, &drawable, &gc, &rects))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    nrects = rects->a_top - rects->a_base;
    if ((xrects = ici_alloc(sizeof (XRectangle) * nrects)) == NULL)
	return 1;
    for (po = rects->a_base, xr = xrects; xr - xrects < nrects; ++xr, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	    ||
	    objof(width = intof(fetch(*po, STRING(width)))) == objof(&o_null)
	    ||
	    !isint(objof(width))
	    ||
	    objof(height = intof(fetch(*po, STRING(height)))) == objof(&o_null)
	    ||
	    !isint(objof(height))
	)
	{
	    error = "invalid rectangle in rectangle array";
	    ici_free(xrects);
	    return 1;
	}
	xr->x = x->i_value;
	xr->y = y->i_value;
	xr->width = width->i_value;
	xr->height = height->i_value;
    }
    XFillRectangles(display->d_display, d, gc->g_gc, xrects, nrects);
    ici_free(xrects);
    return null_ret();
}

FUNC(FillPolygon)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*points;
    int			npoints;
    long		shape;
    long		mode;
    XPoint		*xpoints;
    XPoint		*xp;
    object_t		**po;
    int_t		*x, *y;

    if (ici_typecheck("oooaii", &display, &drawable, &gc, &points, &shape, &mode))
	return 1;

    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    npoints = points->a_top - points->a_base;
    if ((xpoints = ici_alloc(sizeof (XPoint) * npoints)) == NULL)
	return 1;
    for (po = points->a_base, xp = xpoints; xp - xpoints < npoints; ++xp, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	)
	{
	    error = "invalid point in point array";
	    ici_free(xpoints);
	    return 1;
	}
	xp->x = x->i_value;
	xp->y = y->i_value;
    }
    XFillPolygon(display->d_display, d, gc->g_gc, xpoints, npoints, shape, mode);
    ici_free(xpoints);
    return null_ret();
}

FUNC(FillArc)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y;
    long		width, height;
    long		angle1, angle2;

    if (ici_typecheck("oooiiiiii", &display, &drawable, &gc,
			&x, &y, &width, &height, &angle1, &angle2))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    XFillArc(display->d_display, d, gc->g_gc, x, y, width, height, angle1, angle2);
    return null_ret();
}

FUNC(FillArcs)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    array_t		*arcs;
    object_t		**po;
    int			narcs;
    XArc		*xarcs;
    XArc		*xa;
    int_t		*x, *y, *width, *height, *a1, *a2;

    NEED_STRINGS(1);

    if (ici_typecheck("oooa", &display, &drawable, &gc, &arcs))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    narcs = arcs->a_top - arcs->a_base;
    if ((xarcs = ici_alloc(sizeof (XArc) * narcs)) == NULL)
	return 1;
    for (po = arcs->a_base, xa = xarcs; xa - xarcs < narcs; ++xa, ++po)
    {
	if
	(
	    !isstruct(*po)
	    ||
	    objof(x = intof(fetch(*po, STRING(x)))) == objof(&o_null)
	    ||
	    !isint(objof(x))
	    ||
	    objof(y = intof(fetch(*po, STRING(y)))) == objof(&o_null)
	    ||
	    !isint(objof(y))
	    ||
	    objof(width = intof(fetch(*po, STRING(width)))) == objof(&o_null)
	    ||
	    !isint(objof(width))
	    ||
	    objof(height = intof(fetch(*po, STRING(height)))) == objof(&o_null)
	    ||
	    !isint(objof(height))
	    ||
	    objof(a1 = intof(fetch(*po, STRING(angle1)))) == objof(&o_null)
	    ||
	    !isint(objof(a1))
	    ||
	    objof(a2 = intof(fetch(*po, STRING(angle2)))) == objof(&o_null)
	    ||
	    !isint(objof(a2))
	)
	{
	    error = "invalid arc in arc array";
	    ici_free(xarcs);
	    return 1;
	}
	xa->x = x->i_value;
	xa->y = y->i_value;
	xa->width = width->i_value;
	xa->height = height->i_value;
	xa->angle1 = a1->i_value;
	xa->angle2 = a2->i_value;
    }
    XFillArcs(display->d_display, d, gc->g_gc, xarcs, narcs);
    ici_free(xarcs);
    return null_ret();
}

FUNC(DrawString)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    ici_X_gc_t		*gc;
    long		x, y;
    string_t		*s;

    if (ici_typecheck("oooiio", &display, &drawable, &gc, &x, &y, &s))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    if (!isgc(objof(gc)))
	return ici_argerror(2);
    if (!isstring(objof(s)))
	return ici_argerror(5);
    XDrawString(display->d_display, d, gc->g_gc, x, y, s->s_chars, s->s_nchars);
    return null_ret();
}

#include "ici.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "gc.h"

static unsigned long
mark_pixmap(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_pixmap_t);
}

type_t	ici_X_pixmap_type =
{
    mark_pixmap,
    free_simple,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_simple,
    "pixmap"
};

ici_X_pixmap_t *
ici_X_new_pixmap(Pixmap pixmap)
{
    ici_X_pixmap_t	*p;

    if ((p = talloc(ici_X_pixmap_t)) != NULL)
    {
	objof(p)->o_type  = &ici_X_pixmap_type;
	objof(p)->o_tcode = TC_OTHER;
	objof(p)->o_flags = 0;
	objof(p)->o_nrefs = 1;
	rego(objof(p));
	p->p_pixmap = pixmap;
    }
    return p;
}

FUNC(CreatePixmap)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    long		width, height, depth;
    Pixmap		pixmap;

    if (ici_typecheck("ooiii", &display, &drawable, &width, &height, &depth))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    pixmap = XCreatePixmap(display->d_display, d, width, height, depth);
    return ici_ret_with_decref(objof(ici_X_new_pixmap(pixmap)));
}

FUNC(FreePixmap)
{
    ici_X_display_t	*display;
    ici_X_pixmap_t	*pixmap;

    if (ici_typecheck("oo", &display, &pixmap))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!ispixmap(objof(pixmap)))
	return ici_argerror(1);
    if (pixmap->p_pixmap == 0)
#ifdef FREE_SPIRIT
	return null_ret();
#else
    {
	error = "trying to free a pixmap that is already free";
	return 1;
    }
#endif
    XFreePixmap(display->d_display, pixmap->p_pixmap);
    pixmap->p_pixmap = 0;
    return null_ret();
}

FUNC(CreatePixmapFromBitmapData)
{
    ici_X_display_t	*display;
    object_t		*drawable;
    Drawable		d;
    mem_t		*data;
    long		width, height, depth;
    long		fg, bg;
    Pixmap		pixmap;

    if
    (
	ici_typecheck
	(
	    "oomiiiii",
	    &display,
	    &drawable,
	    &data,
	    &width,
	    &height,
	    &fg,
	    &bg,
	    &depth
	)
    )
    {
	return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(drawable))
	d = windowof(drawable)->w_window;
    else if (ispixmap(drawable))
	d = pixmapof(drawable)->p_pixmap;
    else
	return ici_argerror(1);
    pixmap = XCreatePixmapFromBitmapData
    (
	display->d_display,
	d,
	data->m_base,
	width, height,
	fg, bg,
	depth
    );
    return ici_ret_with_decref(objof(ici_X_new_pixmap(pixmap)));
}

FUNC(CopyArea)
{
    ici_X_display_t	*display;
    object_t		*src;
    Drawable		srcd;
    object_t		*dst;
    Drawable		dstd;
    ici_X_gc_t		*gc;
    long		srcx, srcy;
    long		w, h;
    long		dstx, dsty;

    if (NARGS() == 8)
    {
	if (ici_typecheck("ooooiiii", &display, &src, &dst, &gc, &srcx, &srcy, &w, &h))
	    return 1;
	dstx = srcx;
	dsty = srcy;
    }
    else
    {
	if (ici_typecheck("ooooiiiiii", &display, &src, &dst, &gc, &srcx, &srcy, &w, &h, &dstx, &dsty))
	    return 1;
    }

    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (iswindow(src))
	srcd = windowof(src)->w_window;
    else if (ispixmap(src))
	srcd = pixmapof(src)->p_pixmap;
    else
	return ici_argerror(1);
    if (iswindow(dst))
	dstd = windowof(dst)->w_window;
    else if (ispixmap(dst))
	dstd = pixmapof(dst)->p_pixmap;
    else
	return ici_argerror(2);
    if (!isgc(objof(gc)))
	return ici_argerror(3);
    XCopyArea(display->d_display, srcd, dstd, gc->g_gc, srcx, srcy, w, h, dstx, dsty);
    return null_ret();
}

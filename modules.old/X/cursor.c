#include "ici.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "cursor.h"
#include "color.h"

static unsigned long
mark_cursor(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_cursor_t);
}

type_t	ici_X_cursor_type =
{
    mark_cursor,
    free_simple,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_simple,
    "cursor"
};

ici_X_cursor_t *
ici_X_new_cursor(Cursor cursor)
{
    ici_X_cursor_t	*c;

    if ((c = talloc(ici_X_cursor_t)) != NULL)
    {
	objof(c)->o_type  = &ici_X_cursor_type;
	objof(c)->o_tcode = TC_OTHER;
	objof(c)->o_flags = 0;
	objof(c)->o_nrefs = 1;
	rego(objof(c));
	c->c_cursor = cursor;
    }
    return c;
}

FUNC(CreateFontCursor)
{
    ici_X_display_t	*display;
    long		shape;
    Cursor		cursor;

    if (ici_typecheck("oi", &display, &shape))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    cursor = XCreateFontCursor(display->d_display, shape);
    return ici_ret_with_decref(objof(ici_X_new_cursor(cursor)));
}

FUNC(CreatePixmapCursor)
{
    ici_X_display_t	*display;
    ici_X_pixmap_t	*source;
    ici_X_pixmap_t	*mask;
    ici_X_color_t	*foreground_color;
    ici_X_color_t	*background_color;
    long		x_hot, y_hot;
    Cursor		cursor;

    if
    (
	ici_typecheck
	(
	    "oooooii",
	    &display,
	    &source,
	    &mask,
	    &foreground_color,
	    &background_color,
	    &x_hot,
	    &y_hot
	)
    )
    {
	return 1;
    }
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!ispixmap(objof(source)))
	return ici_argerror(1);
    if (!ispixmap(objof(mask)))
	return ici_argerror(2);
    if (!iscolor(objof(foreground_color)))
	return ici_argerror(3);
    if (!iscolor(objof(background_color)))
	return ici_argerror(3);
    cursor = XCreatePixmapCursor
    (
	display->d_display,
	source->p_pixmap,
	mask->p_pixmap,
	&foreground_color->c_color,
	&background_color->c_color,
	x_hot, y_hot
    );
    return ici_ret_with_decref(objof(ici_X_new_cursor(cursor)));
}

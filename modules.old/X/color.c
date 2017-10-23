#include "ici.h"
#include "display.h"
#include "window.h"
#include "color.h"
#include "colormap.h"

static unsigned long
mark_color(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_color_t);
}

NEED_STRING(pixel);
NEED_STRING(red);
NEED_STRING(green);
NEED_STRING(blue);

static object_t *
fetch_color(object_t *o, object_t *k)
{
    NEED_STRINGS(NULL);
    if (k == objof(STRING(pixel)))
	return objof(new_int(colorof(o)->c_color.pixel));
    if (k == objof(STRING(red)))
	return objof(new_int(colorof(o)->c_color.red));
    if (k == objof(STRING(green)))
	return objof(new_int(colorof(o)->c_color.green));
    if (k == objof(STRING(blue)))
	return objof(new_int(colorof(o)->c_color.blue));
    return objof(&o_null);
}

type_t	ici_X_color_type =
{
    mark_color,
    free_simple,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_color,
    "color"
};

ici_X_color_t *
ici_X_new_color(XColor *color)
{
    ici_X_color_t	*c;

    if ((c = talloc(ici_X_color_t)) != NULL)
    {
	objof(c)->o_type  = &ici_X_color_type;
	objof(c)->o_tcode = TC_OTHER;
	objof(c)->o_flags = 0;
	objof(c)->o_nrefs = 1;
	rego(objof(c));
	c->c_color = *color;
    }
    return c;
}

/* Extension - create a color object */
FUNC(Color)
{
    char		*name;
    long		r, g, b;
    XColor		xcolor;

    memset(&xcolor, 0, sizeof xcolor);
    if (NARGS() == 1)
    {
	if (ici_typecheck("s", &name))
	    return 1;
	error = "named color intialisation not implemented";
	return 1;
    }
    else
    {
	if (ici_typecheck("iii", &r, &g, &b))
	    return 1;
	xcolor.red = r;
	xcolor.green = g;
	xcolor.blue = b;
    }
    return ici_ret_with_decref(objof(ici_X_new_color(&xcolor)));
}

NEED_STRING(screen);
NEED_STRING(exact);

FUNC(LookupColor)
{
    ici_X_display_t	*display;
    ici_X_colormap_t	*colormap;
    char		*colorname;
    struct_t		*rc;
    XColor		rgb_db_def;
    XColor		hardware_def;
    ici_X_color_t	*c;

    NEED_STRINGS(1);
    if (ici_typecheck("oos", &display, &colormap, &colorname))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iscolormap(objof(colormap)))
	return ici_argerror(1);
    if
    (
	!XLookupColor
	(
	    display->d_display,
	    colormap->c_colormap,
	    colorname,
	    &rgb_db_def,
	    &hardware_def
	)
    )
    {
	error = "no such color";
	return 1;
    }
    if ((rc = new_struct()) == NULL)
	return 1;
    if ((c = ici_X_new_color(&hardware_def)) == NULL)
	goto fail;
    if (assign(rc, STRING(screen), c))
    {
	decref(c);
	goto fail;
    }
    decref(c);
    if ((c = ici_X_new_color(&rgb_db_def)) == NULL)
	goto fail;
    if (assign(rc, STRING(exact), c))
    {
	decref(c);
	goto fail;
    }
    decref(c);
    return ici_ret_with_decref(objof(rc));

fail:
    decref(rc);
    return 1;
}

FUNC(AllocColor)
{
    ici_X_display_t	*display;
    ici_X_colormap_t	*colormap;
    ici_X_color_t	*color;
    XColor		xcolor;

    if (ici_typecheck("ooo", &display, &colormap, &color))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iscolormap(objof(colormap)))
	return ici_argerror(1);
    if (!iscolor(objof(color)))
	return ici_argerror(2);
    xcolor = color->c_color;
    if (!XAllocColor(display->d_display, colormap->c_colormap, &xcolor))
    {
	error = "failed to allocate color";
	return 1;
    }
    return ici_ret_with_decref(objof(ici_X_new_color(&xcolor)));
}

FUNC(AllocNamedColor)
{
    ici_X_display_t	*disp;
    ici_X_colormap_t	*cm;
    char		*name;
    XColor		scrn;
    XColor		exct;
    struct_t		*rc;
    ici_X_color_t	*c;

    NEED_STRINGS(1);
    if (ici_typecheck("oos", &disp, &cm, &name))
	return 1;
    if (!isdisplay(objof(disp)))
	return ici_argerror(0);
    if (!iscolormap(objof(cm)))
	return ici_argerror(1);
    if (!XAllocNamedColor(disp->d_display, cm->c_colormap, name, &scrn, &exct))
    {
	error = "failed to allocate named color";
	return 1;
    }
    if ((rc = new_struct()) == NULL)
	return 1;
    if ((c = ici_X_new_color(&scrn)) == NULL)
	goto fail;
    if (assign(rc, STRING(screen), c))
    {
	decref(c);
	goto fail;
    }
    decref(c);
    if ((c = ici_X_new_color(&exct)) == NULL)
	goto fail;
    if (assign(rc, STRING(exact), c))
    {
	decref(c);
	goto fail;
    }
    decref(c);
    return ici_ret_with_decref(objof(rc));

fail:
    decref(rc);
    return 1;
}

FUNC(ParseColor)
{
    ici_X_display_t	*display;
    ici_X_colormap_t	*colormap;
    char		*name;
    XColor		xcolor;

    if (ici_typecheck("oos", &display, &colormap, &name))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!iscolormap(objof(colormap)))
	return ici_argerror(1);
    if (!XParseColor(display->d_display, colormap->c_colormap, name, &xcolor))
    {
	error = "bad color";
	return 1;
    }
    return ici_ret_with_decref(objof(ici_X_new_color(&xcolor)));
}

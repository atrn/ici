#include "ici.h"
#include "display.h"
#include "colormap.h"

static unsigned long
mark_colormap(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_colormap_t);
}

static unsigned long
hash_colormap(object_t *o)
{
    return (unsigned long)colormapof(o)->c_colormap * UNIQUE_PRIME;
}

static int
cmp_colormap(object_t *a, object_t *b)
{
    return a != b || colormapof(a)->c_colormap != colormapof(b)->c_colormap;
}

type_t	ici_X_colormap_type =
{
    mark_colormap,
    free_simple,
    hash_colormap,
    cmp_colormap,
    copy_simple,
    assign_simple,
    fetch_simple,
    "colormap"
};

INLINE
static ici_X_colormap_t *
atom_colormap(Colormap colormap)
{
    ici_X_colormap_t	proto = {OBJ(TC_OTHER, ici_X_colormap_type)};

    proto.c_colormap = colormap;
    return colormapof(atom_probe(objof(&proto)));
}

ici_X_colormap_t *
ici_X_new_colormap(Colormap colormap)
{
    ici_X_colormap_t	*c;

    if ((c = atom_colormap(colormap)) != NULL)
	return c;
    if ((c = talloc(ici_X_colormap_t)) != NULL)
    {
	objof(c)->o_type  = &ici_X_colormap_type;
	objof(c)->o_tcode = TC_OTHER;
	objof(c)->o_flags = 0;
	objof(c)->o_nrefs = 1;
	rego(objof(c));
	c->c_colormap = colormap;
    }
    return colormapof(atom(objof(c), 1));
}

FUNC(DefaultColormap)
{
    ici_X_display_t	*display;
    long		screen;
    Colormap		cmap;

    if (ici_typecheck("oi", &display, &screen))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    cmap = DefaultColormap(display->d_display, screen);
    return ici_ret_with_decref(objof(ici_X_new_colormap(cmap)));
}

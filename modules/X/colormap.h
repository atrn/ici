#ifndef ICI_X_COLORMAP_H
#define	ICI_X_COLORMAP_H

/*
 * $Id: colormap.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_colormap
{
    object_t		o_head;
    Colormap		c_colormap;
};

extern type_t		ici_X_colormap_type;

#define	colormapof(o)	((ici_X_colormap_t *)(o))
#define	iscolormap(o)	((o)->o_type == &ici_X_colormap_type)

ici_X_colormap_t	*ici_X_new_colormap(Colormap);

#endif

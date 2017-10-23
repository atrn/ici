#ifndef ICI_X_PIXMAP_H
#define	ICI_X_PIXMAP_H

/*
 * $Id: pixmap.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_pixmap
{
    object_t		o_head;
    Pixmap		p_pixmap;
};

extern type_t		ici_X_pixmap_type;

#define	pixmapof(o)	((ici_X_pixmap_t *)(o))
#define	ispixmap(o)	((o)->o_type == &ici_X_pixmap_type)

ici_X_pixmap_t	*ici_X_new_pixmap(Pixmap);

#endif

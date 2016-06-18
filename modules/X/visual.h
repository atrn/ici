#ifndef ICI_X_VISUAL_H
#define	ICI_X_VISUAL_H

/*
 * $Id: visual.h,v 1.1.1.1 1999/09/08 01:27:24 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_visual
{
    object_t		o_head;
    Visual		*v_visual;
};

extern type_t		ici_X_visual_type;

#define	visualof(o)	((ici_X_visual_t *)(o))
#define	isvisual(o)	((o)->o_type == &ici_X_visual_type)

ici_X_visual_t	*ici_X_new_visual(Visual *);
struct_t	*ici_X_visualinfo_to_struct(XVisualInfo *);
long		ici_X_struct_to_visualinfo(struct_t *, XVisualInfo *);

#endif

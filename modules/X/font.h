#ifndef ICI_X_FONT_H
#define	ICI_X_FONT_H

/*
 * $Id: font.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_font
{
    object_t		o_head;
    Font		f_font;
};

extern type_t		ici_X_font_type;

#define	fontof(o)	((ici_X_font_t *)(o))
#define	isfont(o)	((o)->o_type == &ici_X_font_type)

ici_X_font_t		*ici_X_new_font(Font);

struct ici_X_fontstruct
{
    object_t		o_head;
    XFontStruct		f_fontstruct;
};

extern type_t		ici_X_fontstruct_type;

#define	fontstructof(o)	((ici_X_fontstruct_t *)(o))
#define	isfontstruct(o)	((o)->o_type == &ici_X_fontstruct_type)

ici_X_fontstruct_t	*ici_X_new_fontstruct(XFontStruct *);

#endif

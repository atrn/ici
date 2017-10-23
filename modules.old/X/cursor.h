#ifndef ICI_X_CURSOR_H
#define	ICI_X_CURSOR_H

/*
 * $Id: cursor.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_cursor
{
    object_t		o_head;
    Cursor		c_cursor;
};

extern type_t		ici_X_cursor_type;

#define	cursorof(o)	((ici_X_cursor_t *)(o))
#define	iscursor(o)	((o)->o_type == &ici_X_cursor_type)

ici_X_cursor_t	*ici_X_new_cursor(Cursor);

#endif

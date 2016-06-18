#ifndef ICI_X_WINDOW_H
#define	ICI_X_WINDOW_H

/*
 * $Id: window.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_window
{
    object_t		o_head;
    Window		w_window;
};

extern type_t		ici_X_window_type;

#define	windowof(o)	((ici_X_window_t *)(o))
#define	iswindow(o)	((o)->o_type == &ici_X_window_type)

ici_X_window_t	*ici_X_new_window(Window);

#endif

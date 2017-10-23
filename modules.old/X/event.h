#ifndef ICI_X_EVENT_H
#define	ICI_X_EVENT_H

/*
 * $Id: event.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#ifndef ICI_X_DEFS_H
#include "defs.h"
#endif

struct ici_X_event
{
    object_t	o_head;
    XEvent	*e_event;
    struct_t	*e_struct;
};

extern type_t		ici_X_event_type;

#define	eventof(o)	((ici_X_event_t *)(o))
#define	isevent(o)	((o)->o_type == &ici_X_event_type)

ici_X_event_t		*ici_X_new_event(XEvent *);
long			ici_X_event_mask_for(string_t *);
long			ici_X_event_for(string_t *);
struct_t		*ici_X_event_to_struct(XEvent *);

#endif

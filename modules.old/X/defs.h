#ifndef	ICI_X_DEFS_H
#define	ICI_X_DEFS_H

/*
 * $Id: defs.h,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

typedef struct ici_X_display	ici_X_display_t;
typedef struct ici_X_window	ici_X_window_t;
typedef struct ici_X_visual	ici_X_visual_t;
typedef struct ici_X_gc		ici_X_gc_t;
typedef struct ici_X_atom	ici_X_atom_t;
typedef struct ici_X_event	ici_X_event_t;
typedef struct ici_X_pixmap	ici_X_pixmap_t;
typedef struct ici_X_cursor	ici_X_cursor_t;
typedef struct ici_X_color	ici_X_color_t;
typedef struct ici_X_colormap	ici_X_colormap_t;
typedef struct ici_X_keysym	ici_X_keysym_t;
typedef struct ici_X_font	ici_X_font_t;
typedef struct ici_X_fontstruct	ici_X_fontstruct_t;

/*
 * Use inline functions if available
 */
#if __MSC
#define	INLINE	__inline
#endif

#if __GNUC__
#define	INLINE	__inline__
#endif

#ifndef INLINE
#define	INLINE
#endif

void		ici_X_log(const char *, ...);
Drawable	ici_X_drawable(object_t *);

#endif	/* #ifndef ICI_X_DEFS_H */

#include <stdio.h>
#include <stdarg.h>

#include "ici.h"
#include "display.h"
#include "window.h"
#include "pixmap.h"
#include "gc.h"

void
ici_X_log(const char *fmt, ...)
{
#ifdef DEBUG
    va_list	va;

    fprintf(stderr, "ici:X: ");
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
#endif
}

Drawable
ici_X_drawable(object_t *o)
{
    if (iswindow(o))
	return windowof(o)->w_window;
    if (ispixmap(o))
        return pixmapof(o)->p_pixmap;
    return ~0;
}

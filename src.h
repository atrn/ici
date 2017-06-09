// -*- mode:c++ -*-

#ifndef ICI_SRC_H
#define ICI_SRC_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * The C struct which is the ICI src object. These are never seen by
 * ICI script code. They are source line markers that are passed to
 * debugger functions to indicate source location.
 *
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_src : ici_obj
{
    ici_src() : ici_obj(ICI_TC_SRC), s_lineno(0), s_filename(nullptr) {}
    int         s_lineno;
    ici_str_t   *s_filename;
};
/*
 * s_filename           The name of the source file this source
 *                      marker is associated with.
 *
 * s_lineno             The linenumber.
 *
 * --ici-api-- continued.
 */
#define ici_srcof(o)        ((ici_src_t *)o)
#define ici_issrc(o)        ((o)->o_tcode == ICI_TC_SRC)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_SRC_H */

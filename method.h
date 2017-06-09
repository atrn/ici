// -*- mode:c++ -*-

#ifndef ICI_METHOD_H
#define ICI_METHOD_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct ici_method : ici_obj
{
    ici_obj_t   *m_subject;
    ici_obj_t   *m_callable;
};
#define ici_methodof(o)     ((ici_method_t *)(o))
#define ici_ismethod(o)     (ici_objof(o)->o_tcode == ICI_TC_METHOD)

/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_CFUNC_H */

// -*- mode:c++ -*-

#ifndef ICI_RESTORER_H
#define ICI_RESTORER_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

struct restorer : object
{
    ici_obj_t *(*r_fn)(ici_archive_t *);
};

inline restorer_t *restorerof(ici_obj_t *obj) { return (restorer_t *)obj; }

} // namespace ici

#endif /* ICI_RESTORER_H */

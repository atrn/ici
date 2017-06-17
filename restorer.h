// -*- mode:c++ -*-

#ifndef ICI_RESTORER_H
#define ICI_RESTORER_H

#include "object.h"

namespace ici
{

struct restorer : object
{
    ici_obj_t *(*r_fn)(ici_archive_t *);
};

inline restorer_t *restorerof(ici_obj_t *obj) { return (restorer_t *)obj; }

class restorer_type : public type
{
public:
    restorer_type() : type("restorer", sizeof (struct restorer)) {}
};

} // namespace ici

#endif /* ICI_RESTORER_H */

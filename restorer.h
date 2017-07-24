// -*- mode:c++ -*-

#ifndef ICI_RESTORER_H
#define ICI_RESTORER_H

#include "object.h"

namespace ici
{

struct restorer : object
{
    object *(*r_fn)(archiver *);
};

inline restorer *restorerof(object *o) { return o->as<restorer>(); }

class restorer_type : public type
{
public:
    restorer_type() : type("restorer", sizeof (struct restorer)) {}
};

} // namespace ici

#endif /* ICI_RESTORER_H */

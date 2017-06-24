// -*- mode:c++ -*-

#ifndef ICI_NULL_H
#define ICI_NULL_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
struct null : object
{
    null() : object{ICI_TC_NULL, ICI_O_ATOM, 1, 0} {}
};

inline null *nullof(object *o) { return static_cast<null *>(o); }
inline bool isnull(object *o) { return o == ici_null; }

/*
 * End of ici.h export. --ici.h-end--
 */
class null_type : public type
{
public:
    null_type() : type("NULL", sizeof (struct null)) {}
    void free(ici_obj_t *) override;
};

} // namespace ici

#endif /* ICI_NULL_H */

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
    null() : object{TC_NULL, O_ATOM, 1, 0} {}
};

inline null *nullof(object *o) { return o->as<null>(); }
inline bool isnull(object *o) { return o == ici_null; }

/*
 * End of ici.h export. --ici.h-end--
 */
class null_type : public type
{
public:
    null_type() : type("NULL", sizeof (struct null)) {}
    void free(object *) override;
    int save(archiver *, object *) override;
    object *restore(archiver *) override;
};

} // namespace ici

#endif /* ICI_NULL_H */

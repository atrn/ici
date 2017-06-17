// -*- mode:c++ -*-

#ifndef ICI_SAVER_H
#define ICI_SAVER_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

struct saver : object
{
    int (*s_fn)(ici_archive_t *, ici_obj_t *);
};


inline saver_t *saverof(ici_obj_t *obj) { return (saver_t *)obj; }

class saver_type : public type
{
public:
    saver_type() : type("saver", sizeof (struct saver)) {}
};

} // namespace ici

#endif /* ICI_SAVER_H */

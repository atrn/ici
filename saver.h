// -*- mode:c++ -*-

#ifndef ICI_SAVER_H
#define ICI_SAVER_H

#include "object.h"
#include "type.h"

namespace ici
{

struct saver : object
{
    int (*s_fn)(archiver *, object *);
};


inline saver *saverof(object *obj) { return static_cast<saver *>(obj); }

class saver_type : public type
{
public:
    saver_type() : type("saver", sizeof (struct saver)) {}
};

} // namespace ici

#endif /* ICI_SAVER_H */

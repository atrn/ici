// -*- mode:c++ -*-

#ifndef ICI_CHANNEL_H
#define ICI_CHANNEL_H

#include "object.h"

namespace ici
{

/*
 * This --struct-- forms part of the --ici-api--.
 */
struct channel : objwsup
{
    array *  c_q;
    size_t   c_capacity;
    object * c_altobj;
};

inline channel *channelof(object *o) { return o->as<channel>(); }
inline bool ischannel(object *o) { return o->isa(TC_CHANNEL); }

/*
 * End of ici.h export. --ici.h-end--
 */

class channel_type : public type
{
public:
    channel_type() : type("channel", sizeof (struct channel)) {}
    size_t mark(object *o) override;
};

} // namespace ici

#endif

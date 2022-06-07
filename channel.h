// -*- mode:c++ -*-

#ifndef ICI_CHANNEL_H
#define ICI_CHANNEL_H

#include "object.h"

namespace ici
{

/*
 * This --struct-- forms part of the --ici-api--.
 */
struct channel : object
{
    array  *c_q;
    size_t  c_capacity;
    object *c_altobj;
};

inline channel *channelof(object *o)
{
    return o->as<channel>();
}
inline bool ischannel(object *o)
{
    return o->hastype(TC_CHANNEL);
}

constexpr int ICI_CHANNEL_CLOSED = 0x20;

class channel_type : public type
{
  public:
    channel_type()
        : type("channel", sizeof(struct channel))
    {
    }
    size_t  mark(object *o) override;
    int     forall(object *o) override;
    int     save(archiver *, object *) override;
    object *restore(archiver *) override;
    int64_t len(object *) override;
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

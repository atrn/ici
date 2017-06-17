// -*- mode:c++ -*-

#ifndef ICI_CHANNEL_H
#define ICI_CHANNEL_H

#include "object.h"

namespace ici
{

/*
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_channel : object
{
    ici_array_t *       c_q;
    size_t              c_capacity;
    ici_obj_t *         c_altobj;
};

typedef struct ici_channel ici_channel_t;

inline ici_channel_t *ici_channelof(ici_obj_t *o) { return static_cast<ici_channel_t *>(o); }
inline bool ici_ischannel(ici_obj_t *o) { return o->isa(ICI_TC_CHANNEL); }

/*
 * End of ici.h export. --ici.h-end--
 */

class channel_type : public type
{
public:
    channel_type() : type("channel", sizeof (struct ici_channel)) {}
    size_t mark(ici_obj_t *o) override;
};

} // namespace ici

#endif

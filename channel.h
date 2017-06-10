// -*- mode:c++ -*-

#ifndef ICI_CHANNEL_H
#define ICI_CHANNEL_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

/*
 * This --struct-- forms part of the --ici-api--.
 */
struct ici_channel : ici_obj
{
    ici_array_t *       c_q;
    long                c_capacity;
    ici_obj_t *         c_altobj;
};

typedef struct ici_channel ici_channel_t;

#define ici_channelof(o) (static_cast<ici_channel_t *>(o))
#define ici_ischannel(o) ((o)->o_tcode == ICI_TC_CHANNEL)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif

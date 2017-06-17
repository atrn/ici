// -*- mode:c++ -*-

#ifndef ICI_TYPES_H
#define ICI_TYPES_H

#include "fwd.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * types[] is the "map" from the small integer type codes found in the
 * o_tcode field of every object header, to an instance of a type class
 * that characterises that type of object.  Standard core data types
 * have standard positions (see ICI_TC_* defines in object.h).  Other
 * types are registered at run-time by calls to ici::register_type()
 * and are given the next available slot.  ici_object_t's o_tcode is
 * one byte so we're limited to 256 distinct types.
 */
constexpr size_t        max_types = 256;
extern DLI type_t *     types[max_types];

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

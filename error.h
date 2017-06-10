// -*- mode:c++ -*-

#ifndef ICI_ERROR_H
#define ICI_ERROR_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * This --macro-- forms part of the --ici-api--.
 */
#define                 ICI_MAX_ERROR_MSG       (1024)

/*
 * This --function-- forms part of the --ici-api--.
 */
int                     ici_set_error(const char *, ...);

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_ERROR_H */

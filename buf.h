// -*- mode:c++ -*-

#ifndef ICI_BUF_H
#define ICI_BUF_H

#include "fwd.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

extern int      ici_growbuf(size_t);

/*
 * Ensure that 'ici_buf' points to enough memory to hold index 'n' (plus
 * room for a nul char at the end). Returns 0 on success, else 1 and
 * sets 'ici_error'.
 *
 * See also: 'The error return convention'.
 *
 * This --func-- forms part of the --ici-api--.
 */
inline int ici_chkbuf(size_t n) { return ici_bufz > n ? 0 : ici_growbuf(n); }

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * We use buf as an abbreviation for ici_buf in the core.
 */
#define buf     ici_buf

} // namespace ici

#endif /* ICI_BUF_H */

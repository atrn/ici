// -*- mode:c++ -*-

#ifndef ICI_WRAP_H
#define ICI_WRAP_H

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct wrap
{
    ici_wrap_t  *w_next;
    void        (*w_func)();
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif

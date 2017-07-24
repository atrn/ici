// -*- mode:c++ -*-

#ifndef ICI_RE_H
#define ICI_RE_H

#include "object.h"

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
#ifndef ICI_PCRE_TYPES_DEFINED
typedef void pcre;
typedef void pcre_extra;
#define ICI_PCRE_TYPES_DEFINED
#endif
/*
 * End of ici.h export. --ici.h-end--
 */

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

struct regexp : object
{
    pcre        *r_re;
    pcre_extra  *r_rex;
    str         *r_pat;
};

inline regexp *regexpof(object *o) { return o->as<regexp>(); }
inline bool isregexp(object *o) { return o->isa(TC_REGEXP); }

int ici_pcre_exec_simple(regexp *, str *);

int ici_pcre
(
    regexp *r,
    const char *subject,
    int length,
    int start_offset,
    int options,
    int *offsets,
    int offsetcount
);

/*
 * End of ici.h export. --ici.h-end--
 */

class regexp_type : public type
{
public:
    regexp_type() : type("regexp", sizeof (struct regexp)) {}

    size_t mark(object *o) override;
    void free(object *o) override;
    unsigned long hash(object *o) override;
    int cmp(object *o1, object *o2) override;
    object *fetch(object *o, object *k) override;
};

} // namespace ici

#endif

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
    ici_str_t   *r_pat;
};

inline ici_regexp_t *ici_regexpof(object *o) { return static_cast<ici_regexp_t *>(o); }
inline bool ici_isregexp(object *o) { return o->isa(ICI_TC_REGEXP); }

int ici_pcre_exec_simple(ici_regexp_t *, ici_str_t *);

int ici_pcre
(
    ici_regexp_t *r,
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

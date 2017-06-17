// -*- mode:c++ -*-

#ifndef ICI_RE_H
#define ICI_RE_H

#ifndef ICI_OBJECT_H
#include "object.h"
#endif

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

inline ici_regexp_t *ici_regexpof(ici_obj_t *o) { return static_cast<ici_regexp_t *>(o); }
inline bool ici_isregexp(ici_obj_t *o) { return o->isa(ICI_TC_REGEXP); }

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

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

} // namespace ici

#endif

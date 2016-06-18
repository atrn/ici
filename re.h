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

struct ici_regexp
{
    ici_obj_t   o_head;
    pcre        *r_re;
    pcre_extra  *r_rex;
    ici_str_t   *r_pat;
};

#define ici_regexpof(o)     ((ici_regexp_t *)(o))
#define ici_isregexp(o)     ((o)->o_tcode == ICI_TC_REGEXP)

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

#endif

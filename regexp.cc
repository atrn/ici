#define ICI_CORE
#include "fwd.h"
#include "str.h"
#include "re.h"
#include "exec.h"
#include "op.h"
#include "buf.h"
#include "primes.h"

#ifndef ICI
#define ICI /* Cause PCRE's internal.h to add special stuff for ICI */
#endif
#include "internal.h"

namespace ici
{

/*
 * Called when the above regexp routines detect an error.
 * Space for the offsets to matched sub-expressions in regexps and a global
 * counter for the number of matches. PCRE says the number of offsets must be
 * a multiple of three. See pcre.3 for an explanation.
 */

int     re_bra[(nsubexp + 1) * 3];
int     re_nbra;

/*
 * Return a new ICI regxep compiled from the given string 's'. 'flags'
 * may contain PCRE option settings as descriped in man pcre.3
 *
 * The returned object has had its reference count incremented.
 *
 * Returns NULL on error, usual conventions.
 */
regexp *new_regexp(str *s, int flags)
{
    regexp     *r;
    pcre       *re;
    pcre_extra *rex = NULL;
    int         errofs;

    /* Special test for possible failure of ici_str_new_nul_term() in lex.c */
    if (s == NULL)
        return NULL;
    re = pcre_compile(s->s_chars, flags, (const char **)&error, &errofs, NULL);
    if (re == NULL)
        return NULL;
    if (pcre_info(re, NULL, NULL) > nsubexp)
    {
        set_error("too many subexpressions in regexp, limit is %d", nsubexp);
        goto fail;
    }
    rex = pcre_study(re, 0, (const char **)&error);
    if (error != NULL)
        goto fail;
    /* Note rex can be NULL if no extra info required */
    if ((r = (regexp *)ici_talloc(regexp)) == NULL)
        goto fail;
    ICI_OBJ_SET_TFNZ(r, ICI_TC_REGEXP, 0, 1, 0);
    r->r_re  = re;
    r->r_rex = rex;
    r->r_pat = stringof(atom(s, 0));
    ici_rego(r);
    return regexpof(atom(r, 1));

 fail:
    if (rex != NULL)
        ici_free(rex);
    if (re != NULL)
        ici_free(re);
    return NULL;
}

/*
 * This function is just a wrapper round pcre_exec so that external modules
 * don't need to drag in the whole definition of pcre's include files.
 */
int
ici_pcre(regexp *r,
         const char *subject, int length, int start_offset,
         int options, int *offsets, int offsetcount)
{
    return pcre_exec
    (
        r->r_re,
        r->r_rex,
        subject,
        length,
        start_offset,
        options,
        offsets,
        offsetcount
    );
}

int ici_pcre_exec_simple(regexp *r, str *s)
{
    return pcre_exec
    (
	r->r_re,
	r->r_rex,
	s->s_chars,
	s->s_nchars,
	0,
	0,
	re_bra,
	nels(re_bra)
    );
}

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
size_t regexp_type::mark(object *o)
{
    return type::mark(o) + ici_mark(regexpof(o)->r_pat) + ((real_pcre *)regexpof(o)->r_re)->size;
}

/*
 * Free this object and associated memory (but not other objects).
 * See the comments on t_free() in object.h.
 */
void regexp_type::free(object *o)
{
    if (regexpof(o)->r_rex != NULL)
        ici_free(regexpof(o)->r_rex);
    ici_free(regexpof(o)->r_re);
    ici_tfree(o, regexp);
}

/*
 * Return a hash sensitive to the value of the object.
 * See the comment on t_hash() in object.h
 */
unsigned long regexp_type::hash(object *o)
{
    /* static unsigned long     primes[] = {0xBF8D, 0x9A4F, 0x1C81, 0x6DDB}; */
    int re_options;
    pcre_info(regexpof(o)->r_re, &re_options, NULL);
    return ((unsigned long)regexpof(o)->r_pat + re_options) * 0x9A4F;
}

/*
 * Returns 0 if these objects are equal, else non-zero.
 * See the comments on t_cmp() in object.h.
 */
int regexp_type::cmp(object *o1, object *o2)
{
    int re1_options;
    int re2_options;
    pcre_info(regexpof(o1)->r_re, &re1_options, NULL);
    pcre_info(regexpof(o2)->r_re, &re2_options, NULL);
    return re1_options != re2_options ? 1 : ici_cmp(regexpof(o1)->r_pat, regexpof(o2)->r_pat);
}

object *regexp_type::fetch(object *o, object *k)
{
    if (k == SS(pattern))
        return regexpof(o)->r_pat;
    if (k == SS(options))
    {
        int       options;
        ici_int   *io;

        pcre_info(regexpof(o)->r_re, &options, NULL);
        if ((io = new_int(options)) == NULL)
            return NULL;
        io->decref();
        return io;
    }
    return fetch_fail(o, k);
}

} // namespace ici

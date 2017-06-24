#define ICI_CORE
#include "exec.h"
#include "func.h"
#include "str.h"
#include "int.h"
#include "float.h"
#include "struct.h"
#include "buf.h"
#include "re.h"
#include "null.h"
#include "op.h"
#include "array.h"

#include <stdio.h>
#include <ctype.h>

#include "pcre.h"

namespace ici
{

static int
f_regexp(...)
{
    int opts = 0;

    switch (NARGS())
    {
    case 2:
        if (!isint(ARG(1)))
            return argerror(1);
        opts = intof(ARG(1))->i_value;
        /* FALLTHROUGH */
    case 1:
        if (!isstring(ARG(0)))
            return argerror(0);
        break;
    default:
        return argcount(2);
    }
    if (ICI_CF_ARG2() != NULL)
        opts |= PCRE_CASELESS;
    return ret_with_decref(ici_regexp_new(stringof(ARG(0)), opts));
}

/*
 * Some internal macros used by the functions below. Local variable s is
 * assumed. Return the start (end) address of the n'th matched sub-pattern,
 * or the whole match for n == 0.
 */
#define START(n) (s + re_bra[(n) * 2])
#define END(n)   (s + re_bra[(n) * 2 + 1])

/*
 * do_repl()
 *
 * Generate into the buffer pointed to by 'd' the replacement text defined
 * by the 'repl' with substitutions made from the current match on 's'.
 * 'replz' is number of chars in repl.
 *
 * Returns the length of the string stored into the buffer. If d is NULL,
 * no storage into the buffer is done, but the length calculation still is.
 */
static int
do_repl
(
    char        *s,
    char        *repl,
    int         replz,
    char        *d
)
{
    char        *reple;
    int         dz;
    int         normal;

    /*
     * Copy across the replacement expression.
     */
    for (dz = 0, normal = 1, reple = repl + replz; repl < reple; ++repl)
    {
        int c = *repl;
        if (normal)
        {
            if (c == '\\')
                normal = 0;
            else
            {
                if (d != NULL)
                    d[dz] = c;
                ++dz;
            }
        }
        else
        {
            normal = 1;
            if (c == '0')
            {
                if (d != NULL)
                    memcpy(&d[dz], s, START(0)- s);
                dz += START(0) - s;
            }
            else if (c == '&')
            {
                if (d != NULL)
                    memcpy(&d[dz], START(0), END(0) - START(0));
                dz += END(0) - START(0);
            }
            else if (c == '\\')
            {
                if (d != NULL)
                    d[dz] = '\\';
                ++dz;
            }
            else if (!isdigit(c))
            {
                if (d != NULL)
                {
                    d[dz] = '\\';
                    d[dz + 1] = c;
                }
                dz += 2;
            }
            else if (START(c -= '0') != NULL)
            {
                if (d != NULL)
                    memcpy(&d[dz], START(c), END(c) - START(c));
                dz += END(c) - START(c);
            }
        }
    }
    return dz;
}


/*
 * do_smash()
 *
 * Return an array containing all the expanded replacement strings generated
 * by repeatedly applying the regular expression re to the string str (moving
 * down to the unmatched portion each time). The replacement strings
 * (repls[], of which there are n_repls, indexed 0, -1, -2...) may contain
 * \0 indicating the unmatched portion, \& the whole matched portions and
 * \1, \2... being the mached sub-bracketed portions.
 *
 * Return an array, or NULL on error. The array has been increfed.
 */
static array *do_smash
(
    str   *thestr,
    regexp *re,
    str   **repls,
    int     n_repls,
    int     include_remainder
)
{
    char  *s;                   /* Where we are up to in the string. */
    char  *se;                  /* The end of the string. */
    int    i;
    array *a;
    str   *ns;
    int    size;

    if ((a = ici_array_new(0)) == NULL)
        goto fail;
    for (s = thestr->s_chars, se = s + thestr->s_nchars; ; s = END(0))
    {
        /*
         * Match the regexp against the input string.
         */
        if
        (
            !pcre_exec
            (
                re->r_re,
                re->r_rex,
                s,
                se - s,
                0,
                s > thestr->s_chars ? PCRE_NOTBOL : 0,
                re_bra,
                nels(re_bra)
            )
            ||
            END(0) == START(0) /* Match, but no progress. */
        )
            break;

        /*
         * Generate the new strings and push them onto the array.
         */
        for (i = 0; i < n_repls; ++i)
        {
            if (a->stk_push_chk())
               goto fail;
            size = do_repl(s, repls[-i]->s_chars, repls[-i]->s_nchars, NULL);
            if ((ns = ici_str_alloc(size)) == NULL)
                goto fail;
            do_repl(s, repls[-i]->s_chars, repls[-i]->s_nchars, ns->s_chars);
            if ((ns = stringof(atom(ns, 1))) == NULL)
                goto fail;
            *a->a_top++ = ns;
            ns->decref();
        }
    }
    if (include_remainder && s != se)
    {
        /*
         * There is left-over un-matched string. Push it, as a string onto
         * the array too.
         */
        if (a->stk_push_chk())
            goto fail;
        if ((ns = ici_str_new(s, se - s)) == NULL)
            goto fail;
        *a->a_top++ = ns;
        ns->decref();
    }
    return a;

fail:
    if (a != NULL)
        a->decref();
    return NULL;
}

/*
 * Perform a single regexp substitution for sub() or gsub(). The
 * parameters are,
 *
 *      str     The ICI string object to substitute something within.
 *      re      The regexp to match against str to find what to
 *              substitute.
 *      repl    The replacement text.
 *      ofs     (Pointer to) The offset within str where we wish to
 *              start the match. This is used by gsub() to avoid
 *              matching what it has replaced (which can cause infinite
 *              loops).
 */
static str *do_sub(str *thestr, regexp *re, char *repl, int *ofs)
{
    char *dst;
    int   normal;
    char *p;
    str  *rc;
    int   len;
    char *d;
    char *s;

    s = thestr->s_chars + *ofs;

    /*
     * Match the regexp against the input string.
     */
    if
    (
        !pcre_exec
        (
            re->r_re,
            re->r_rex,
            s,
            thestr->s_nchars - *ofs,
            0,
            *ofs > 0 ? PCRE_NOTBOL : 0,
            re_bra,
            nels(re_bra)
        )
    )
    {
        return NULL;
    }


    /*
     * This is a bit gratuitous.  Detect stupid substitutions like
     *          gsub("x", "q*$", "")
     * for which the correct behaviour is to loop infinitely.  Don't.
     */
    if (END(0) == START(0))
        return NULL;

    /*
     * The string is divided into three parts. The bit before the matched
     * regexp, the matched section and anything that follows. We want to
     * determine the size of the actual output string so we can allocate
     * some space for it. Initially we know the size of the areas that
     * aren't within the match.
     */
    len = thestr->s_nchars - (END(0) - START(0)) + 1;

    /*
     * Determine size of matched area. This depends on the replacement
     * text. If there are any \n (n is a digit or ampersand) sequences
     * these must be replaced by the appropriate section of the input string.
     */
    for (normal = 1, p = repl; *p != 0; ++p)
    {
        int c = *p;

        if (normal)
        {
            if (c == '\\')
                normal = 0;
            else
                ++len;
        }
        else
        {
            normal = 1;
            if (c == '&')
                len += END(0) - START(0);
            else if (c == '\\')
                ++len;
            else if (!isdigit(c))
                len += 2;
            else if (START(c -= '0') != NULL)
                len += END(c) - START(c);
        }
    }

    /*
     * Now get that much space and stuff it with the string. The "+1" for
     * the NUL character at the end of the string.
     */
    if ((dst = (char *)ici_alloc(len + 1)) == NULL)
        return (str *)-1;

    /*
     * Copy across the part of the source as far as the start of the match.
     */
    memcpy(dst, thestr->s_chars, START(0) - thestr->s_chars);
    d = &dst[START(0) - thestr->s_chars];

    /*
     * Copy across the replacement expression.
     */
    for (normal = 1, p = repl; *p != 0; ++p)
    {
        int c = *p;
        if (normal)
        {
            if (c == '\\')
                normal = 0;
            else
                *d++ = c;
        }
        else
        {
            normal = 1;
            if (c == '&')
            {
                memcpy(d, START(0), END(0) - START(0));
                d += END(0) - START(0);
                *d = '\0';
            }
            else if (c == '\\')
                *d++ = '\\';
            else if (!isdigit(c))
            {
                *d++ = '\\';
                *d++ = c;
            }
            else if (START(c -= '0') != NULL)
            {
                memcpy(d, START(c), END(c) - START(c));
                d += END(c) - START(c);
                *d = '\0';
            }
        }
    }

    /*
     * Update offset where last replacement finished. If we get called
     * again (from f_gsub()) it knows not to replace into something we've
     * already replaced (possible infinite loop there).
     */
    *ofs = d - dst;

    /*
     * So far, `dst' is just the replaced string.  Now copy across
     * the remainder of the source (from the end of the match onwards).
     */
    len = (thestr->s_chars + thestr->s_nchars) - END(0) + 1;
    memcpy(d, END(0), len);
    d[len] = '\0';
    rc = ici_str_new_nul_term(dst);
    ici_free(dst);
    if (rc == NULL)
        return (str *)-1;
    return rc;
}

#undef START
#undef END

static int
f_sub(...)
{
    object   *thestr;
    object   *o;
    regexp    *re;
    char        *repl;
    str   *rc;
    int         ofs = 0;

    /*
     * Get the ICI arguments.
     */
    if (typecheck("oos", &thestr, &o, &repl))
        return 1;
    if (!isstring(thestr))
        return argerror(0);
    if (isregexp(o))
        re = regexpof(o);
    else if (!isstring(o))
        return argerror(1);
    else if ((re = ici_regexp_new(stringof(o), 0)) == NULL)
        return 1;
    if ((rc = do_sub(stringof(thestr), re, repl, &ofs)) == NULL)
        rc = stringof(thestr);
    else if (rc == (str *)-1)
    {
        if (regexpof(o) != re)
            re->decref();
        return 1;
    }
    else
        rc->decref();

    if (regexpof(o) != re)
        re->decref();

    return ret_no_decref(rc);
}

static int
f_gsub(...)
{

    str           *thestr;
    regexp  *re;
    str           *repl;
    str           *repls[2];
    char          *s;
    object       **p;
    int            size;
    str           *ns;
    array         *a;

    /*
     * Get the ICI arguments.
     */
    a = NULL;
    if (NARGS() < 3)
        return argcount(3);
    if (!isstring(ARG(0)))
        return argerror(0);
    thestr = stringof(ARG(0));
    if (!isstring(ARG(2)))
        return argerror(2);
    repl = stringof(ARG(2));
    if (!isregexp(ARG(1)))
    {
        if (!isstring(ARG(1)))
            return argerror(1);
        if ((re = ici_regexp_new(stringof(ARG(1)), 0)) == NULL)
            return 1;
    }
    else
        re = regexpof(ARG(1));

    repls[0] = repl;
    repls[1] = SS(slosh0);
    if ((a = do_smash(thestr, re, &repls[1], 2, 1)) == NULL)
        goto fail;
    for (p = a->a_base, size = 0; p < a->a_top; ++p)
        size += stringof(*p)->s_nchars;
    if ((ns = ici_str_alloc(size)) == NULL)
        goto fail;
    for (p = a->a_base, s = ns->s_chars; p < a->a_top; ++p)
    {
        memcpy(s, stringof(*p)->s_chars, stringof(*p)->s_nchars);
        s += stringof(*p)->s_nchars;
    }
    if ((ns = stringof(atom(ns, 1))) == NULL)
        goto fail;
    a->decref();
    if (!isregexp(ARG(1)))
        re->decref();
    return ret_with_decref(ns);

fail:
    if (a != NULL)
        a->decref();
    if (!isregexp(ARG(1)))
        re->decref();
    return 1;
}

/*
 * Return the number of pointers in a NULL terminated array of pointers.
 */
static int
nptrs(char **p)
{
    int        i;

    i = 0;
    while (*p++ != NULL)
        ++i;
    return i;
}

/*
 * The original smash() function which we would like to phase out.
 */
static int
f_old_smash()
{
    char   *s;
    char   *delim;
    char  **p;
    array  *sa;
    char  **strs;

    if (typecheck("ss", &s, &delim))
        return 1;
    if (delim[0] == 0)
    {
        return set_error("bad delimiter string");
    }
    if (strlen(delim) == 1)
        strs = smash(s, delim[0]);
    else
        strs = ssmash(s, delim);
    if (strs == NULL)
        return 1;
    if ((sa = ici_array_new(nptrs(strs))) == NULL)
        goto fail;
    for (p = strs; *p != NULL; ++p)
    {
        if ((*sa->a_top = ici_str_get_nul_term(*p)) == NULL)
            goto fail;
        ++sa->a_top;
    }
    ici_free((char *)strs);
    return ret_with_decref(sa);

fail:
    if (sa != NULL)
        sa->decref();
    ici_free((char *)strs);
    return 1;
}

regexp *smash_default_re;

/*
 * f_smash()
 *
 * Implementation of the ICI smash() function.
 */
static int
f_smash(...)
{
    str           *thestr;
    regexp  *re;
    str          **repls;
    int            n_repls;
    array         *a;
    str           *default_repl[2];
    int            i;
    int            include_remainder;
    int            nargs;

    if (NARGS() < 1)
        return argcount(1);

    if (NARGS() == 2 && isstring(ARG(1)))
        return f_old_smash();

    nargs = NARGS();
    include_remainder = 0;
    if (isint(ARG(nargs - 1)))
    {
        include_remainder = intof(ARG(NARGS() - 1))->i_value != 0;
        if (--nargs == 0)
            return argerror(0);
    }

    if (!isstring(ARG(0)))
        return argerror(0);
    thestr = stringof(ARG(0));

    if (nargs < 2)
    {
        if (smash_default_re == NULL)
        {
            if ((smash_default_re = ici_regexp_new(SS(sloshn), 0)) == NULL)
                return 1;
        }
        re = smash_default_re;
    }
    else
    {
        if (!isregexp(ARG(1)))
            return argerror(1);
        re = regexpof(ARG(1));
    }

    if (nargs < 3)
    {
        default_repl[0] = SS(slosh0);
        repls = &default_repl[0];
        n_repls = 1;
    }
    else
    {
        n_repls = nargs - 2;
        repls = (str **)(ARGS() - 2);
        for (i = 0; i < n_repls; ++i)
        {
            if (!isstring(repls[-i]))
                return argerror(i + 2);
        }
    }

    a = do_smash(thestr, re, repls, n_repls, include_remainder);
    return ret_with_decref(a);
}

ICI_DEFINE_CFUNCS(re)
{
    ICI_DEFINE_CFUNC(regexp,       f_regexp),
    ICI_DEFINE_CFUNC2(regexpi,     f_regexp,       NULL,   (void *)""),
    ICI_DEFINE_CFUNC(sub,          f_sub),
    ICI_DEFINE_CFUNC(gsub,         f_gsub),
    ICI_DEFINE_CFUNC(smash,        f_smash),
    ICI_CFUNCS_END()
};

} // namespace ici

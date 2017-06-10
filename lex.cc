#define ICI_CORE
#include "fwd.h"
#include "parse.h"
#include "file.h"
#include "buf.h"
#include "src.h"
#include "str.h"
#include "array.h"

namespace ici
{

/*
 * Set this to non-zero to stop the recording of file and line number
 * information as code is parsed.  There is nothing in the interpreter core
 * that sets this.  Setting this can both save memory and increase execution
 * speed (slightly).  But diagnostics won't report line numbers and source
 * line debugging operations won't work.
 *
 * This --variable-- forms part of the --ici-api--.
 */
int     ici_dont_record_line_nums;

/*
 * Return the next character from the file being parsed in the given parse
 * context p. This cooks the input to normalise various newlines conventions
 * (\n \r and \r\n) into a single \n. It also tracks the current line number
 * and, if the code array a is supplied, updates or appends a source line
 * and file object at the end of the array. Also maintains the start of line
 * flag (p_sol) in the parse context.
 */
static int
get(ici_parse_t *p, ici_array_t *a)
{
    int         c;

    if ((c = (*p->p_file->f_type->ft_getch)(p->p_file->f_file)) == '\n' || c == '\r')
    {
        if (c == '\n' && p->p_sol && p->p_cr)
        {
            /*
             * This is a \n after after a \r.  That is regarded as just one
             * newline.  Get the next character.
             */
            c = (*p->p_file->f_type->ft_getch)(p->p_file->f_file);
            if (c == '\n' || c == '\r')
            {
                ++p->p_lineno;
                p->p_cr = c == '\r';
                c = '\n';
            }
            else
            {
                p->p_sol = 0;
            }
        }
        else
        {
            p->p_sol = 1;
            ++p->p_lineno;
            p->p_cr = c == '\r';
            c = '\n';
        }
    }
    else
    {
        p->p_sol = 0;
    }

    if (a != NULL && !ici_dont_record_line_nums)
    {
        /*
         * There is a code array being built. Update any trailing
         * source marker, and if there isn't one, add one.
         */
        if (a->a_top > a->a_base && ici_issrc(a->a_top[-1]))
        {
            ici_srcof(a->a_top[-1])->s_lineno = p->p_lineno;
        }
        else if (ici_stk_push_chk(a, 1) == 0)
        {
            if ((*a->a_top = ici_src_new(p->p_lineno, p->p_file->f_name)) != NULL)
            {
                ici_decref(*a->a_top);
                ++a->a_top;
            }
        }
    }

    return c;
}

/*
 * Unget the character c from the parse context p. Tries to behave as if
 * the character had never been fetched.
 */
static void
unget(int c, ici_parse_t *p)
{
    (*p->p_file->f_type->ft_ungetch)(c, p->p_file->f_file);
    if (c == '\n')
    {
        --p->p_lineno;
    }
}

/*
 * Return the next token from the file being parsed in the given parse
 * context p. If the code array a is supplied, updates or appends a
 * source line and file object at the end of the array. Returns T_ERORR
 * on error, in which case error is set.
 */
int
ici_lex(ici_parse_t *p, ici_array_t *a)
{
    int                 c;
    int                 t = 0; /* init to shut up compiler */
    int                 i;
    int                 fstate;
    char                *s;
    long                l;
    double              d;

    if (p->p_got.t_what & TM_HASOBJ)
    {
        /*
         * No-one consumed the object reference in the last token we returned.
         * Disgard it now. This only happens because of user parseing, as ICI's
         * parser is always well behaved and consumes tokens completely before
         * getting the next one.
         */
        ici_decref(p->p_got.t_obj);
        p->p_got.t_what = T_NONE;
    }

    /*
     * Skip white space, in its various forms.
     */
    for (;;)
    {
        i = p->p_sol;
        if ((c = get(p, a)) == '#' && i)
        {
            while ((c = get(p, a)) != '\n' && c != EOF)
            {
                ;
            }
            continue;
        }
        else if (c == '\n')
        {
            continue;
        }

        if (c == '/')
        {
            if ((c = get(p, a)) == '*')
            {
                /*
                 * A traditional C comment.
                 */
                while ((c = get(p, a)) != EOF)
                {
                    if (c == '*')
                    {
                        if ((c = get(p, a)) == '/')
                        {
                            break;
                        }
                        unget(c, p);
                    }
                }
            }
            else if (c == '/')
            {
                /*
                 * A // style comment.
                 */
                while ((c = get(p, a)) != EOF && c != '\n')
                {
                    ;
                }
            }
            else
            {
                unget(c, p);
                goto slash;
            }
            continue;
        }

        if (c != ' ' && c != '\t')
        {
            break;
        }
    }

    /*
     * Decypher the next token.
     */
    switch (c)
    {
    case '/':
    slash:
        if ((c = get(p, a)) == '=')
        {
            t = T_SLASHEQ;
        }
        else
        {
            unget(c, p);
            t = T_SLASH;
        }
        break;

    case EOF:
        t = T_EOF;
        break;

    case '$':
        t = T_DOLLAR;
        break;

    case '@':
        t = T_AT;
        break;

    case '(':
        t = T_ONROUND;
        break;

    case ')':
        t = T_OFFROUND;
        break;

    case '{':
        t = T_ONCURLY;
        break;

    case '}':
        t = T_OFFCURLY;
        break;

    case ',':
        t = T_COMMA;
        break;

    case '~':
        if ((c = get(p, a)) == '~')
        {
            if ((c = get(p, a)) == '~')
            {
                t = T_3TILDE;
            }
            else if (c == '=')
            {
                t = T_2TILDEEQ;
            }
            else
            {
                unget(c, p);
                t = T_2TILDE;
            }
        }
        else
        {
            unget(c, p);
            t = T_TILDE;
        }
        break;

    case '[':
        t = T_ONSQUARE;
        break;

    case ']':
        t = T_OFFSQUARE;
        break;

    case '.':
        if ((c = get(p, a)) >= '0' && c <= '9')
        {
            unget(c, p);
            c = '.';
            i = 0;
            goto alphanum;
        }
        unget(c, p);
        t = T_DOT;
        break;

    case '*':
        if ((c = get(p, a)) == '=')
        {
            t = T_ASTERIXEQ;
        }
        else
        {
            unget(c, p);
            t = T_ASTERIX;
        }
        break;

    case '%':
        if ((c = get(p, a)) == '=')
        {
            t = T_PERCENTEQ;
        }
        else
        {
            unget(c, p);
            t = T_PERCENT;
        }
        break;

    case '^':
        if ((c = get(p, a)) == '=')
        {
            t = T_CARETEQ;
        }
        else
        {
            unget(c, p);
            t = T_CARET;
        }
        break;

    case '+':
        if ((c = get(p, a)) == '=')
        {
            t = T_PLUSEQ;
        }
        else if (c == '+')
        {
            t = T_PLUSPLUS;
        }
        else
        {
            unget(c, p);
            t = T_PLUS;
        }
        break;

    case '-':
        if ((c = get(p, a)) == '>')
        {
            t = T_PTR;
        }
        else if (c == '=')
        {
            t = T_MINUSEQ;
        }
        else if (c == '-')
        {
            t = T_MINUSMINUS;
        }
        else
        {
            unget(c, p);
            t = T_MINUS;
        }
        break;

    case '>':
        if ((c = get(p, a)) == '>')
        {
            if ((c = get(p, a)) == '=')
            {
                t = T_GRTGRTEQ;
            }
            else
            {
                unget(c, p);
                t = T_GRTGRT;
            }
        }
        else if (c == '=')
        {
            t = T_GRTEQ;
        }
        else
        {
            unget(c, p);
            t = T_GRT;
        }
        break;

    case '<':
        if ((c = get(p, a)) == '<')
        {
            if ((c = get(p, a)) == '=')
            {
                t = T_LESSLESSEQ;
            }
            else
            {
                unget(c, p);
                t = T_LESSLESS;
            }
        }
        else if (c == '=')
        {
            if ((c = get(p, a)) == '>')
            {
                t = T_LESSEQGRT;
            }
            else
            {
                unget(c, p);
                t = T_LESSEQ;
            }
        }
        else
        {
            unget(c, p);
            t = T_LESS;
        }
        break;

    case '=':
        if ((c = get(p, a)) == '=')
        {
            t = T_EQEQ;
        }
        else
        {
            unget(c, p);
            t = T_EQ;
        }
        break;

    case '!':
        if ((c = get(p, a)) == '=')
        {
            t = T_EXCLAMEQ;
        }
        else if (c == '~')
        {
            t = T_EXCLAMTILDE;
        }
        else
        {
            unget(c, p);
            t = T_EXCLAM;
        }
        break;

    case '&':
        if ((c = get(p, a)) == '&')
        {
            t = T_ANDAND;
        }
        else if (c == '=')
        {
            t = T_ANDEQ;
        }
        else
        {
            unget(c, p);
            t = T_AND;
        }
        break;

    case '|':
        if ((c = get(p, a)) == '|')
        {
            t = T_BARBAR;
        }
        else if (c == '=')
        {
            t = T_BAREQ;
        }
        else
        {
            unget(c, p);
            t = T_BAR;
        }
        break;

    case ';':
        t = T_SEMICOLON;
        break;

    case '?':
        t = T_QUESTION;
        break;

    case ':':
        if ((c = get(p, a)) == '^')
        {
            t = T_COLONCARET;
        }
        else if (c == '=')
        {
            t = T_COLONEQ;
        }
        else
        {
            unget(c, p);
            t = T_COLON;
        }
        break;

    case '#':
    {
        i = 0;
        while ((c = get(p, a)) != '#' && c != '\n' && c != EOF)
        {
            if (ici_chkbuf(i))
            {
                goto fail;
            }
            buf[i++] = c;
        }
        if (c == '\n')
        {
            ici_set_error("newline in #...#");
            goto fail;
        }
        if ((p->p_got.t_obj = ici_str_new(buf, i)) == NULL)
        {
            goto fail;
        }
        t = T_REGEXP;
        break;
    }
    case '\'':
        t = T_INT;
        goto chars;
    case '\"':
        t = T_STRING;
    chars:
        i = 0;
        while ((c = get(p, a)) != (t == T_INT ? '\'' : '"') && c != '\n' && c!=EOF)
        {
            if (ici_chkbuf(i))
            {
                goto fail;
            }
            if (c == '\\')
            {
                switch (c = get(p, a))
                {
                case '\n': continue;
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'v': c = '\v'; break;
                case 'b': c = '\b'; break;
                case 'r': c = '\r'; break;
                case 'f': c = '\014'; break;
                case 'a': c = '\007'; break;
                case 'e': c = '\033'; break;
                case '\\': break;
                case '\'': break;
                case '"': break;
                case '?': break;

                case 'c':
                    c = get(p, a) & 0x1F;
                    break;

                case 'x':
                    l = 0;
                    while
                    (
                        ((c = get(p, a)) >= '0' && c <= '9')
                        || (c >= 'a' && c <= 'f')
                        || (c >= 'A' && c <= 'F'))
                    {
                        if (c >= 'a' && c <= 'f')
                        {
                            c -= 'a' - 10;
                        }
                        else if (c >= 'A' && c <= 'F')
                        {
                            c -= 'A' - 10;
                        }
                        else
                        {
                            c -= '0';
                        }
                        l = l * 16 + c;
                    }
                    unget(c, p);
                    c = l;
                    break;

                default:
                    if (c >= '0' && c <= '7')
                    {
                        l = c - '0';
                        if ((c = get(p, a)) >= '0' && c <= '7')
                        {
                            l = l * 8 + c - '0';
                            if ((c = get(p, a)) >= '0' && c <= '7')
                            {
                                l = l * 8 + c - '0';
                            }
                            else
                            {
                                unget(c, p);
                            }
                        }
                        else
                        {
                            unget(c, p);
                        }
                        c = (int)l;
                    }
                    else
                    {
                        ici_set_error("unknown \\ escape");
                        goto fail;
                    }
                }
            }
            buf[i++] = c;
            if (t == T_INT)
            {
                if ((c = get(p, a)) != '\'')
                {
                    ici_set_error("too many chars in ' ' sequence");
                    goto fail;
                }
                break;
            }
        }
        if (ici_chkbuf(i))
        {
            goto fail;
        }
        buf[i] = '\0';
        if (t == T_INT)
        {
            if (i == 0)
            {
                ici_set_error("newline in ' '");
                goto fail;
            }
            p->p_got.t_int = buf[0] & 0xFF;
        }
        else
        {
            if (c == '\n')
            {
                ici_set_error("newline in \"...\"");
                goto fail;
            }
            if ((p->p_got.t_obj = ici_str_new(buf, i)) == NULL)
            {
                goto fail;
            }
        }
        break;

    default:
        if
        (
            (c < '0' || c > '9')
            &&
            (c < 'a' || c > 'z')
            &&
            (c < 'A' || c > 'Z')
            &&
            c != '_'
            &&
            c != '.'
        )
        {
            ici_set_error("lexical error");
            goto fail;
        }

/*
 * States to keep track of passage through a floating point number.
 * ddd[.ddd][e|E[+|-]ddd]
 */
#define FS_NOTF         0
#define FS_ININT        1
#define FS_INFRAC       2
#define FS_POSTE        3
#define FS_INEXP        4

        i = 0;
    alphanum:
        fstate = c=='.' ? FS_INFRAC : c>='0' && c<='9' ? FS_ININT : FS_NOTF;
        for (;;)
        {
            if (ici_chkbuf(i))
            {
                goto fail;
            }
            buf[i++] = c;
            c = get(p, a);
            switch (fstate)
            {
            case FS_POSTE:
                if ((c >= '0' && c <= '9') || c == '+' || c == '-')
                {
                    fstate = FS_INEXP;
                    continue;
                }
                goto notf;

            case FS_ININT:
                if (c == '.')
                {
                    fstate = FS_INFRAC;
                    continue;
                }
            case FS_INFRAC:
                if (c == 'e' || c == 'E')
                {
                    fstate = FS_POSTE;
                    continue;
                }
            case FS_INEXP:
                if (c >= '0' && c <= '9')
                {
                    continue;
                }
            notf:
                fstate = FS_NOTF;
            case FS_NOTF:
                if
                (
                    (c >= '0' && c <= '9')
                    ||
                    (c >= 'a' && c <= 'z')
                    ||
                    (c >= 'A' && c <= 'Z')
                    ||
                    c == '_'
                )
                {
                    continue;
                }
                break;
            }
            break;

        }
        unget(c, p);
        if (ici_chkbuf(i))
        {
            break;
        }
        buf[i] = '\0';
        l = ici_strtol(buf, &s, 0);
        if (*s == '\0')
        {
            p->p_got.t_int = l;
            t = T_INT;
            break;
        }
        d = strtod(buf, &s);
        if (*s == '\0')
        {
            p->p_got.t_float = d;
            t = T_FLOAT;
            break;
        }
        if ((p->p_got.t_obj = ici_str_new_nul_term(buf)) == NULL)
        {
            goto fail;
        }
        t = T_NAME;
        break;
    }
    p->p_got.t_what = t;
    return t;

fail:
    p->p_got.t_what = T_ERROR;
    return T_ERROR;
}

/*
 * Functions to support reading currentfile() in cooked mode (which
 * tracks line numbers).
 */
static int
pf_getc(void *file)
{
    return get((ici_parse_t *)file, NULL);
}

static int
pf_ungetc(int c, void *file)
{
    unget(c, (ici_parse_t *)file);
    return c;
}

static int
pf_eof(void *file)
{
    ici_parse_t *p = (ici_parse_t *)file;
    return p->p_file->f_type->ft_eof(p->p_file->f_file);
}

static int
pf_fail(void *p)
{
    return 0;
}

static long
pf_lfail(void *p, long o, int w)
{
    return 0;
}

static int
pf_wfail(const void *p, long o, void * w)
{
    return 0;
}

ici_ftype_t ici_parse_ftype =
{
    0,
    pf_getc,
    pf_ungetc,
    pf_fail,    /* flush */
    pf_fail,    /* close */
    pf_lfail,   /* seek */
    pf_eof,
    pf_wfail    /* write */
};

} // namespace ici

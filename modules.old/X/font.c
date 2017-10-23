/*
 * $Id: font.c,v 1.1.1.1 1999/09/08 01:27:23 andy Exp $
 *
 * A type to represent X11 fonts and functions that operate on them.
 */

#include "ici.h"
#include "display.h"
#include "font.h"

static unsigned long
mark_font(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_font_t);
}

static unsigned long
hash_font(object_t *o)
{
    return (unsigned long)fontof(o)->f_font * UNIQUE_PRIME;
}

static int
cmp_font(object_t *a, object_t *b)
{
    return (a != b) || fontof(a)->f_font != fontof(b)->f_font;
}

type_t	ici_X_font_type =
{
    mark_font,
    free_simple,
    hash_font,
    cmp_font,
    copy_simple,
    assign_simple,
    fetch_simple,
    "font"
};

INLINE
static object_t *
atom_font(Font font)
{
    object_t	**po, *o;

    for
    (
	po = &atoms[ici_atom_hash_index(font * UNIQUE_PRIME)];
	(o = *po) != NULL;
	--po < atoms ? po = atoms + atomsz - 1 : NULL
    )
    {
	if (isfont(o) && fontof(o)->f_font == font)
	    return o;
    }
    return NULL;
}

ici_X_font_t *
ici_X_new_font(Font font)
{
    ici_X_font_t	*f;

    if ((f = fontof(atom_font(font))) != NULL)
    {
	incref(objof(f));
	return f;
    }
    if ((f = talloc(ici_X_font_t)) != NULL)
    {
	objof(f)->o_type  = &ici_X_font_type;
	objof(f)->o_tcode = TC_OTHER;
	objof(f)->o_flags = 0;
	objof(f)->o_nrefs = 1;
	rego(objof(f));
	f->f_font = font;
    }
    return f == NULL ? f : fontof(atom(objof(f), 1));
}

static unsigned long
mark_fontstruct(object_t *o)
{
    o->o_flags |= O_MARK;
    return sizeof (ici_X_fontstruct_t);
}

NEED_STRING(fid);
NEED_STRING(ascent);
NEED_STRING(descent);

static object_t *
fetch_fontstruct(object_t *o, object_t *k)
{
    NEED_STRINGS(NULL);
    if (k == objof(STRING(fid)))
	return objof(ici_X_new_font(fontstructof(o)->f_fontstruct.fid));
    if (k == objof(STRING(ascent)))
	return objof(new_int(fontstructof(o)->f_fontstruct.ascent));
    if (k == objof(STRING(descent)))
	return objof(new_int(fontstructof(o)->f_fontstruct.descent));
    return objof(&o_null);
}

type_t	ici_X_fontstruct_type =
{
    mark_fontstruct,
    free_simple,
    hash_unique,
    cmp_unique,
    copy_simple,
    assign_simple,
    fetch_fontstruct,
    "fontstruct"
};

ici_X_fontstruct_t *
ici_X_new_fontstruct(XFontStruct *fontstruct)
{
    ici_X_fontstruct_t	*f;

    if ((f = talloc(ici_X_fontstruct_t)) != NULL)
    {
	objof(f)->o_tcode = TC_OTHER;
	objof(f)->o_nrefs = 1;
	objof(f)->o_flags = 0;
	objof(f)->o_type  = &ici_X_fontstruct_type;
	rego(f);
	f->f_fontstruct = *fontstruct;
    }
    return f;
}

FUNC(LoadFont)
{
    ici_X_display_t	*display;
    char		*name;
    Font		font;

    if (ici_typecheck("os", &display, &name))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    font = XLoadFont(display->d_display, name);
    return ici_ret_with_decref(objof(ici_X_new_font(font)));
}

FUNC(UnloadFont)
{
    ici_X_display_t	*display;
    ici_X_font_t	*font;

    if (ici_typecheck("oo", &display, &font))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    if (!isfont(objof(font)))
	return ici_argerror(1);
    XUnloadFont(display->d_display, font->f_font);
    return null_ret();
}

FUNC(LoadQueryFont)
{
    ici_X_display_t	*display;
    char		*name;
    XFontStruct		*fontstruct;

    if (ici_typecheck("os", &display, &name))
	return 1;
    if (!isdisplay(objof(display)))
	return ici_argerror(0);
    fontstruct = XLoadQueryFont(display->d_display, name);
    if (fontstruct == NULL)
    {
	error = "failed to load font";
	return 1;
    }
    return ici_ret_with_decref(objof(ici_X_new_fontstruct(fontstruct)));
}

FUNC(TextWidth)
{
    ici_X_fontstruct_t	*fontstruct;
    string_t		*str;

    if (ici_typecheck("oo", &fontstruct, &str))
	return 1;
    if (!isfontstruct(objof(fontstruct)))
	return ici_argerror(0);
    if (!isstring(objof(str)))
	return ici_argerror(1);
    return int_ret(XTextWidth(&fontstruct->f_fontstruct, str->s_chars, str->s_nchars));
}

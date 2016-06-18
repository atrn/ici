/*
 * $Id: xml.c,v 1.11 2003/03/08 06:48:06 timl Exp $
 *
 * xml.c - Interface to the Expat XML parser.
 *
 * Expat is the XML parser written by J Clark. It is the parser being used to
 * add XML support to Netscape 5 and Perl.
 *
 * http://www.jclark.com/xml/expat.html
 *
 * Originally by Scott Newham <snaf@zeta.org.au>
 * Complete rewrite by Andy Newman <atrn@zeta.org.au>
 *                        ^ note difference in the names, we're not related!
 * Another rewrite to make it OO in ICI 4 by Tim Long.
 */

/*
 * Expat based xml parser
 *
 * The ICI xml module provides an ICI interface to James Clark's expat XML
 * parser.  If may be used in a manner equivalent to the C functions that
 * provided callbacks as different items in the XML stream are encounted, or
 * alternatively an XML stream can be read into, and written out from, an ICI
 * data structure.
 *
 * When used in the callback mode of the low level expat module, function
 * names are kept the same (with the 'XML_' becomming 'xml.').  Calling
 * conventions are kept as close to those of the C API as possible. The basic
 * operation in this mode is to sub-class the 'XML_Parser' class and supply
 * any callback methods you wish. For example:
 *
 *  MyXML_Parser := [class:xml.XML_Parser,
 *
 *      start_element(name, attr)
 *      {
 *          ...
 *      }
 *
 *      end_element(name)
 *      {
 *          ...
 *      }
 *  ];
 *
 *  parser = MyXMLParser:new();
 *
 * Then successive text segments from the XML stream can be supplied by
 * repeated calls such as:
 *
 *  parser:Parse(str);
 *
 * which will result in calls to the 'start_element' and 'end_element' as
 * necessary. Other callback methods you may supply are 'character_data',
 * 'processing_instruction' and 'default_handler'.
 *
 * This --intro-- and --synopsis-- are part of --ici-xml-- documentation.
 */

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>
#include <xmlparse.h>
#include <assert.h>

objwsup_t    *ici_xml_module;
objwsup_t    *ici_xml_parser_class;

/*
 * Convert the current error in the given XML_Parser object (p) to
 * an ICI error. Returns 1.
 */
static int
ici_xml_error(XML_Parser p)
{
    char        *msg;

    msg = (char *)XML_ErrorString(XML_GetErrorCode(p));
    if (ici_chkbuf(80))
        ici_error = msg;
    else
    {
        sprintf
        (
            ici_buf,
            "line %d, column %d: %s",
            XML_GetCurrentLineNumber(p),
            XML_GetCurrentColumnNumber(p),
            msg
        );
        ici_error = ici_buf;
    }
    return 1;
}

/*
 * inst:start_element(name, attr)
 *
 * This is a method that you may supply in your sub-class of the XML_Parser class
 * (there is a default implementation that does nothing). It is called at the
 * start of each element that is encounted by the Parse method. The 'name'
 * is a string (the name of the element). 'attr' is a struct with keys and
 * values being the attributes of the element (all strings). There will be a
 * matching call to 'end_element' at the end of the element.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static void
start_element(void *udata, const char *name, const char **atts)
{
    ici_handle_t    *h = udata;
    object_t    *a;

    if (ici_error != NULL)
        return;
    /*
     * The attributes are put into a struct object as string objects. The
     * keys are the attribute names, the values the attribute value. If
     * there are no attributes an empty struct is passed.
     */
    if ((a = ici_objof(ici_struct_new())) == NULL)
        return;
    if (*atts != NULL)
    {
        long            i;
        char            **p;
        string_t        *n;
        string_t        *s;

        for (p = (char **)atts, i = 0; *p != NULL; p += 2, i += 2)
        {
            assert(p[0] != NULL && p[1] != NULL);
            if ((n = ici_str_new_nul_term(p[0])) == NULL)
            {
                ici_decref(a);
                return;
            }
            if ((s = ici_str_new_nul_term(p[1])) == NULL)
            {
                ici_decref(n);
                ici_decref(a);
                return;
            }
            if (ici_assign(a, ici_objof(n), ici_objof(s)))
            {
                ici_decref(s);
                ici_decref(n);
                ici_decref(a);
                return;
            }
            ici_decref(s);
            ici_decref(n);
        }
    }
    ici_method(ici_objof(h), ICIS(start_element), "so", name, a);
    ici_decref(a);
}

/*
 * inst:end_element(name)
 *
 * This is a method that you may supply in your sub-class of the XML_Parser class
 * (there is a default implementation that does nothing). It is called at the
 * end of each element that is encounted by the Parse method.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static void
end_element(void *udata, const char *name)
{
    ici_handle_t    *h = udata;

    if (ici_error != NULL)
        return;
    ici_method(ici_objof(h), ICIS(end_element), "s", name);
}

/*
 * inst:character_data(data)
 *
 * This is a method that you may supply in your sub-class of the XML_Parser class
 * (there is a default implementation that does nothing). It is called with
 * each section of character data that is encounted by the Parse method.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static void
character_data(void *udata, const XML_Char *str, int len)
{
    ici_handle_t        *h = udata;
    string_t        *s;

    if (ici_error != NULL)
        return;
    if ((s = ici_str_new((char *)str, len)) != NULL)
    {
        ici_method(ici_objof(h), ICIS(character_data), "o", s);
        ici_decref(s);
    }
}

/*
 * inst:default_handler(data)
 *
 * This is a method that you may supply in your sub-class of the XML_Parser
 * class (there is a default implementation that does nothing).  It is called
 * with any characters in the XML document for which there is no applicable
 * handler.  This includes both characters that are part of markup which is of
 * a kind that is not reported (comments, markup declarations), or characters
 * that are part of a construct which could be reported but for which no
 * handler has been supplied.  The characters are passed exactly as they were
 * in the XML document except that they will be encoded in UTF-8.  Line
 * boundaries are not normalized.  There are no guarantees about how
 * characters are divided between calls to the default handler: for example, a
 * comment might be split between multiple calls.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static void
default_handler(void *udata, const XML_Char *str, int len)
{
    ici_handle_t            *h = udata;
    string_t            *s;

    if (ici_error != NULL)
        return;
    if ((s = ici_str_new((char *)str, len)) == NULL)
        return;
    ici_method(ici_objof(h), ICIS(default_handler), "o", s);
    ici_decref(s);
}

/*
 * inst:processing_instruction(target, data)
 *
 * This is a method that you may supply in your sub-class of the XML_Parser class
 * (there is a default implementation that does nothing). It is called with
 * each processing instruction that is encounted by the Parse method.
 *
 * Both 'target' and 'data' are strings.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static void
processing_instruction(void *udata, const XML_Char *targ, const XML_Char *data)
{
    ici_handle_t            *h = udata;
    string_t            *t;
    string_t            *d;

    if (ici_error != NULL)
        return;
    if ((t = ici_str_new_nul_term((char *)targ)) == NULL)
        return;
    if ((d = ici_str_new_nul_term((char *)data)) == NULL)
    {
        ici_decref(t);
        return;
    }
    ici_method(ici_objof(h), ICIS(processing_instruction), "oo", t, d);
    ici_decref(t);
    ici_decref(d);
}

/*
 * The handle is about to be freed. Free the parser object.
 */
static void
ici_xml_pre_free(ici_handle_t *h)
{
    XML_ParserFree((XML_Parser)h->h_ptr);
}

/*
 * inst = xml.XML_Parser:new()
 *
 * Create and return a new XML_Parser instance.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_parser_new(objwsup_t *klass)
{
    XML_Parser          p;
    ici_handle_t        *h;

    if (ici_method_check(ici_objof(klass), 0))
        return 1;
    if (ici_typecheck(""))
        return 1;
    if ((p = XML_ParserCreate(NULL)) == NULL)
    {
        ici_error = "failed to create XML parser";
        return 1;
    }
    XML_SetElementHandler(p, start_element, end_element);
    XML_SetCharacterDataHandler(p, character_data);
    XML_SetDefaultHandler(p, default_handler);
    XML_SetProcessingInstructionHandler(p, processing_instruction);
    if ((h = ici_handle_new(p, ICIS(XML_Parser), klass)) == NULL)
        return 1;
    h->h_pre_free = ici_xml_pre_free;
    XML_SetUserData(p, h);
    return ici_ret_with_decref(ici_objof(h));
}

/*
 * inst:Parse(str [, isfinal])
 *
 * Parse some XML source given by 'str' in the XML_Parser instance 'inst'.
 * If 'isfinal' (an int) is true, this is the last segment of input.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_Parse(object_t *inst)
{
    string_t            *s;
    long                is_final;
    XML_Parser          p;
    ici_handle_t        *h;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), &h, &p))
        return 1;
    switch (ICI_NARGS())
    {
    case 1:
        if (ici_typecheck("o", &s))
            return 1;
        is_final = 1;
        break;

    default:
        if (ici_typecheck("oi", &s, &is_final))
            return 1;
    }
    if (!ici_isstring(ici_objof(s)))
        return ici_argerror(1);
    ici_error = NULL;
    if (!XML_Parse(p, s->s_chars, s->s_nchars, is_final))
        return ici_xml_error(p);
    return ici_error != NULL ? 1 : ici_null_ret();
}

/*
 * str = inst:GetErrorCode()
 *
 * Return the current error code from an XML_Parser instance 'inst'.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_GetErrorCode(object_t *inst)
{
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    return ici_int_ret(XML_GetErrorCode(p));
}

/*
 * str = inst:ErrorString()
 *
 * Return the current error string from an XML_Parser instance 'inst'.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_ErrorString(object_t *inst)
{
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    return ici_str_ret((char*)XML_ErrorString(XML_GetErrorCode(p)));
}

/*
 * int = inst:GetCurrentLineNumber()
 *
 * Return the current line number of the XML_Parser instance 'inst'.
 *
 * This --topic-- formas part of the --ici-xml-- documentation.
 */
static int
ici_xml_GetCurrentLineNumber(object_t *inst)
{
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    return ici_int_ret(XML_GetCurrentLineNumber(p));
}

/*
 * int = inst:GetCurrentColumnNumber()
 *
 * Return the current column number of the XML_Parser instance 'inst'.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_GetCurrentColumnNumber(object_t *inst)
{
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    return ici_int_ret(XML_GetCurrentColumnNumber(p));
}

/*
 * int = inst:GetCurrentByteIndex()
 *
 * Return the current byte index of the XML_Parser instance 'inst'.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_GetCurrentByteIndex(object_t *inst)
{
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    return ici_int_ret(XML_GetCurrentByteIndex(p));
}

/*
 * inst:SetBase(str)
 *
 * Sets the base to be used for resolving relative URIs in system identifiers
 * in declarations of the XML_Parser instance 'inst'.  Resolving relative
 * identifiers is left to the application.
 *
 * This --topic-- forms part of the --ici-xml-- documentation.
 */
static int
ici_xml_SetBase(object_t *inst)
{
    char                *s;
    XML_Parser           p;

    if (ici_handle_method_check(inst, ICIS(XML_Parser), NULL, &p))
        return 1;
    if (ici_typecheck("s", &s))
        return 1;
    if (!XML_SetBase(p, (XML_Char *)s))
        return ici_xml_error(p);
    return ici_null_ret();
}

cfunc_t ici_xml_cfuncs[] =
{
    {ICI_CF_OBJ}
};

cfunc_t ici_xml_parser_cfuncs[] =
{
    {ICI_CF_OBJ, "new", ici_xml_parser_new},
    {ICI_CF_OBJ, "SetBase", ici_xml_SetBase},
    {ICI_CF_OBJ, "Parse", ici_xml_Parse},
    {ICI_CF_OBJ, "ErrorString", ici_xml_ErrorString},
    {ICI_CF_OBJ, "GetCurrentByteIndex", ici_xml_GetCurrentByteIndex},
    {ICI_CF_OBJ, "GetCurrentColumnNumber", ici_xml_GetCurrentColumnNumber},
    {ICI_CF_OBJ, "GetCurrentLineNumber", ici_xml_GetCurrentLineNumber},
    {ICI_CF_OBJ, "GetErrorCode", ici_xml_GetErrorCode},
    {ICI_CF_OBJ}
};

object_t *
ici_xml_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "xml"))
        return NULL;
    if (init_ici_str())
        return NULL;
    if ((ici_xml_module = ici_module_new(ici_xml_cfuncs)) == NULL)
        return NULL;
    if ((ici_xml_parser_class = ici_class_new(ici_xml_parser_cfuncs, ici_xml_module)) == NULL)
        goto fail;
    if (ici_assign_base(ici_xml_module, ICIS(parser_class), ici_xml_parser_class))
        goto fail;
    if (ici_assign_base(ici_xml_module, ICIS(XML_Parser), ici_xml_parser_class))
        goto fail;
    ici_decref(ici_xml_parser_class);
    return ici_objof(ici_xml_module);

fail:
    ici_decref(ici_xml_module);
    return NULL;
}

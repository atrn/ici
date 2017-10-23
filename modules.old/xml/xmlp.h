/*
 * ICI bindings for Expat - XML parser by J Clark.
 *
 * http://www.jclark.com/xml/expat.html
 */

#ifndef ICI_XMLP_H
#define ICI_XMLP_H

#include <xmlparse.h>

/*
 * XML parser object.
 */
typedef struct xmlp_t
{
    object_t    o_head;
    XML_Parser  x_parser;
    func_t  *x_startf;
    func_t  *x_endf;
    func_t  *x_charf;
    func_t  *x_defaultf;
    func_t  *x_pinstf;
    object_t    *x_arg;
}
xmlp_t;
/*
 * x_parser Opaque pointer to the Expat parser.
 *
 * x_startf, x_endf
 *      ICI callback functions for the start and end elements or NULL.
 *
 * x_charf  ICI callback function for character data or NULL.
 *
 * x_defaultf   ICI callback function for the default handler or NULL.
 *
 * x_pinstf ICI callback function for processing instructions or NULL.
 *
 * x_arg    ICI object that is the user arg to pass to the callback
 *      functions. Initial value is the null object.
 */

#define xmlpof(o)       ((xmlp_t *)(o))
#define isxmlp(o)       (objof(o)->o_tcode == xmlp_type_code)

extern int              xmlp_type_code;

#endif /* ICI_XMLP_H */

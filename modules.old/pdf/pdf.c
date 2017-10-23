/*
 * PDFlib binding
 *
 * ICI language binding to Thomas Merz's PDFlib PDF generation library.
 *
 * This --intro-- and --synopsis-- are part of --ici-pdf-- documentation.
 */

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>
#include <pdflib.h>

static int
check_if_deleted(ici_handle_t *pdf)
{
    if (objof(pdf)->o_flags & H_CLOSED)
    {
        ici_error = "attempt to use previously deleted PDF object";
        return 1;
    }
    return 0;
}

static char *pdf_error = NULL;

static void
errorproc(PDF *p, int type, const char *msg)
{
    pdf_error = (char *)msg;
}

static void *
allocproc(PDF *p, size_t size, const char *caller)
{
    return malloc(size);
}

static void *
reallocproc(PDF *p, void *ptr, size_t size, const char *caller)
{
    return realloc(ptr, size);
}

static void
freeproc(PDF *p, void *ptr)
{
    free(ptr);
}

static int
pdf_null_ret(void)
{
    if (pdf_error != NULL)
    {
        ici_error = pdf_error;
        pdf_error = NULL;
        return 1;
    }
    return ici_null_ret();
}

static int
pdf_int_ret(long v)
{
    if (pdf_error != NULL)
    {
        ici_error = pdf_error;
        pdf_error = NULL;
        return 1;
    }
    return ici_int_ret(v);
}

static int
pdf_float_ret(double v)
{
    if (pdf_error != NULL)
    {
        ici_error = pdf_error;
        pdf_error = NULL;
        return 1;
    }
    return ici_float_ret(v);
}

static int
pdf_str_ret(const char *s)
{
    if (pdf_error != NULL)
    {
        ici_error = pdf_error;
        pdf_error = NULL;
        return 1;
    }
    return ici_str_ret((char *)s);
}

static int
ici_pdf_get_majorversion(void)
{
    return pdf_int_ret(PDF_get_majorversion());
}

static int
ici_pdf_get_minorversion(void)
{
    return pdf_int_ret(PDF_get_minorversion());
}

static int
ici_pdf_boot(void)
{
    PDF_boot();
    return pdf_null_ret();
}

static int
ici_pdf_shutdown(void)
{
    PDF_shutdown();
    return pdf_null_ret();
}

static void
ici_pdf_pre_free(ici_handle_t *h)
{
    if (!(objof(h)->o_flags & H_CLOSED))
        PDF_delete((PDF *)h->h_ptr);
}

static int
ici_pdf_new(void)
{
    PDF *pdf = PDF_new2(errorproc, allocproc, reallocproc, freeproc, &ici_error);
    ici_handle_t *h;

    if (pdf == NULL)
    {
        if (pdf_error != NULL)
        {
            ici_error = pdf_error;
            pdf_error = NULL;
        }
        else
        {
            ici_error = "unable to make PDF object";
        }
        return 1;
    }
    if ((h = ici_handle_new(pdf, ICIS(PDF), NULL)) == NULL)
    {
        PDF_delete(pdf);
        return 1;
    }
    h->h_pre_free = ici_pdf_pre_free;
    objof(h)->o_flags &= ~O_SUPER;
    return ici_ret_with_decref(objof(h));
}

static int
args_0(ici_handle_t **ppdf)
{
    if (ici_typecheck("h", ICIS(PDF), ppdf))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_0(void (*fn)(PDF *))
{
    ici_handle_t *pdf;

    if (args_0(&pdf))
        return 1;
    (*fn)((PDF *)pdf->h_ptr);
    return pdf_null_ret();
}

static int
args_1s(ici_handle_t **ppdf, char **pstr)
{
    if (ici_typecheck("hs", ICIS(PDF), ppdf, pstr))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_1s(void (*fn)(PDF *, const char *))
{
    ici_handle_t *pdf;
    char *str;
    if (args_1s(&pdf, &str))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, str);
    return pdf_null_ret();
}

static int
args_1d(ici_handle_t **ppdf, long *pa)
{
    if (ici_typecheck("hi", ICIS(PDF), ppdf, pa))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_1d(void (*fn)(PDF *, int))
{
    ici_handle_t *pdf;
    long v;

    if (args_1d(&pdf, &v))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, v);
    return pdf_null_ret();
}

static int
args_1f(ici_handle_t **ppdf, double *pa)
{
    if (ici_typecheck("hn", ICIS(PDF), ppdf, pa))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_1f(void (*fn)(PDF *, float))
{
    ici_handle_t *pdf;
    double v;

    if (args_1f(&pdf, &v))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, v);
    return pdf_null_ret();
}

static int
args_2s(ici_handle_t **ppdf, char **pstr1, char **pstr2)
{
    if (ici_typecheck("hss", ICIS(PDF), ppdf, pstr1, pstr2))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_2s(void (*fn)(PDF *, const char *, const char *))
{
    ici_handle_t *pdf;
    char *str1;
    char *str2;
    if (args_2s(&pdf, &str1, &str2))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, str1, str2);
    return pdf_null_ret();
}

static int
args_2f(ici_handle_t **ppdf, double *pa, double *pb)
{
    if (ici_typecheck("hnn", ICIS(PDF), ppdf, pa, pb))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_2f_i(int (*fn)(PDF *, float, float))
{
    ici_handle_t *pdf;
    double a, b;

    if (args_2f(&pdf, &a, &b))
        return 1;
    return pdf_int_ret((*fn)((PDF *)pdf->h_ptr, a, b));
}

static int
pdf_2f(void (*fn)(PDF *, float, float))
{
    ici_handle_t *pdf;
    double a, b;

    if (args_2f(&pdf, &a, &b))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, a, b);
    return pdf_null_ret();
}

static int
args_3f(ici_handle_t **ppdf, double *pa, double *pb, double *pc)
{
    if (ici_typecheck("hnnn", ICIS(PDF), ppdf, pa, pb, pc))
        return 1;
    return check_if_deleted(*ppdf);
}

static int
pdf_3f(void (*fn)(PDF *, float, float, float))
{
    ici_handle_t *pdf;
    double a, b, c;

    if (args_3f(&pdf, &a, &b, &c))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, a, b, c);
    return pdf_null_ret();
}

static int
args_1s1fo(ici_handle_t **ppdf, char **pstr, double *pfloat)
{
    if (NARGS() == 2)
    {
        if (args_1s(ppdf, pstr))
            return 1;
        *pfloat = 0.0;
    }
    else if (NARGS() == 3)
    {
        if (ici_typecheck("hsn", ICIS(PDF), ppdf, pstr, pfloat))
            return 1;
    }
    else
    {
        return ici_argcount(2);
    }
    return check_if_deleted(*ppdf);
}

static int
ici_pdf_delete(void)
{
    ici_handle_t *pdf;

    if (args_0(&pdf))
        return 1;
    PDF_delete((PDF *)pdf->h_ptr);
    objof(pdf)->o_flags |= H_CLOSED;
    return pdf_null_ret();
}

static int
ici_pdf_open_file(void)
{
    ici_handle_t *pdf;
    char *filename;

    if (args_1s(&pdf, &filename))
        return 1;
    if (!PDF_open_file((PDF *)pdf->h_ptr, filename))
    {
        ici_error = "unable to open file";
        return 1;
    }
    return pdf_null_ret();
}

static int
ici_pdf_close(void)
{
    ici_handle_t *pdf;

    if (args_0(&pdf))
        return 1;
    PDF_close((PDF *)pdf->h_ptr);
    return pdf_null_ret();
}

static int
ici_pdf_begin_page(void)
{
    return pdf_2f(PDF_begin_page);
}

static int
ici_pdf_end_page(void)
{
    return pdf_0(PDF_end_page);
}

static int
ici_pdf_findfont(void)
{
    ici_handle_t *pdf;
    char *fontname;
    char *encoding;
    long embed;
    int fn;

    if (ici_typecheck("hssi", ICIS(PDF), &pdf, &fontname, &encoding, &embed))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    fn = PDF_findfont((PDF *)pdf->h_ptr, fontname, encoding, embed);
    return pdf_int_ret(fn);
}

static int
ici_pdf_setfont(void)
{
    ici_handle_t *pdf;
    long  fn;
    double fontsize;

    if (ici_typecheck("hin", ICIS(PDF), &pdf, &fn, &fontsize))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_setfont((PDF *)pdf->h_ptr, fn, fontsize);
    return pdf_null_ret();
}

static int
ici_pdf_show(void)
{
    return pdf_1s(PDF_show);
}

static int
ici_pdf_show_xy(void)
{
    ici_handle_t *pdf;
    char *str;
    double x, y;

    if (ici_typecheck("hsnn", ICIS(PDF), &pdf, &str, &x, &y))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_show_xy((PDF *)pdf->h_ptr, str, x, y);
    return pdf_null_ret();
}

static int
ici_pdf_continue_text(void)
{
    return pdf_1s(PDF_continue_text);
}

static int
ici_pdf_show_boxed(void)
{
    ici_handle_t *pdf;
    char *str, *hmode, *feature;
    double x, y, w, h;

    if (ici_typecheck("hsnnnnss", ICIS(PDF), &pdf, &str, &x, &y, &w, &h, &hmode, &feature))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    return pdf_int_ret(PDF_show_boxed((PDF *)pdf->h_ptr, str, x, y, w, h, hmode, feature));
}

static int
pdf_6f(void (*fn)(PDF *, float, float, float, float, float, float))
{
    ici_handle_t *pdf;
    double a, b, c, d, e, f;

    if (ici_typecheck("hnnnnnn", ICIS(PDF), &pdf, &a, &b, &c, &d, &e, &f))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, a, b, c, d, e, f);
    return pdf_null_ret();
}

static int
pdf_5f(void (*fn)(PDF *, float, float, float, float, float))
{
    ici_handle_t *pdf;
    double a, b, c, d, e;

    if (ici_typecheck("hnnnnn", ICIS(PDF), &pdf, &a, &b, &c, &d, &e))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, a, b, c, d, e);
    return pdf_null_ret();
}

static int
pdf_4f(void (*fn)(PDF *, float, float, float, float))
{
    ici_handle_t *pdf;
    double a, b, c, d;

    if (ici_typecheck("hnnnn", ICIS(PDF), &pdf, &a, &b, &c, &d))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    (*fn)((PDF *)pdf->h_ptr, a, b, c, d);
    return pdf_null_ret();
}

static int
ici_pdf_set_text_matrix(void)
{
    return pdf_6f(PDF_set_text_matrix);
}

static int
ici_pdf_set_text_pos(void)
{
    return pdf_2f(PDF_set_text_pos);
}

static int
ici_pdf_string_width(void)
{
    ici_handle_t *pdf;
    char *str;
    long fn;
    double fsz;

    if (ici_typecheck("hsin", ICIS(PDF), &pdf, &str, &fn, &fsz))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    return pdf_float_ret(PDF_stringwidth((PDF *)pdf->h_ptr, str, fn, fsz));
}

static int
ici_pdf_setdash(void)
{
    return pdf_2f(PDF_setdash);
}

static int
ici_pdf_setpolydash(void)
{
    ici_handle_t *pdf;
    array_t *a;
    float  default_array[8];
    float *farray = default_array;
    int    i, length;

    if (ici_typecheck("ha", ICIS(PDF), &pdf, &a))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    length = a->a_top - a->a_base;
    if (length > (sizeof default_array / sizeof (float)))
    {
        farray = ici_alloc(length * sizeof (float));
        if (farray == NULL)
            return 1;
    }
    for (i = 0; i < length; ++i)
    {
        if (!isfloat(a->a_base[i]))
        {
            ici_error = "type other than float in dash vector";
            return 1;
        }
        farray[i] = (float)floatof(a->a_base[i])->f_value;
    }
    PDF_setpolydash((PDF *)pdf->h_ptr, farray, length);
    if (farray != default_array)
        ici_free(farray);
    return pdf_null_ret();
}

static int
ici_pdf_setflat(void)
{
    return pdf_1f(PDF_setflat);
}

static int
ici_pdf_setlinejoin(void)
{
    return pdf_1d(PDF_setlinejoin);
}

static int
ici_pdf_setlinecap(void)
{
    return pdf_1d(PDF_setlinecap);
}

static int
ici_pdf_setmiterlimit(void)
{
    return pdf_1f(PDF_setmiterlimit);
}

static int
ici_pdf_setlinewidth(void)
{
    return pdf_1f(PDF_setlinewidth);
}

static int
ici_pdf_initgraphics(void)
{
    return pdf_0(PDF_initgraphics);
}

static int
ici_pdf_save(void)
{
    return pdf_0(PDF_save);
}

static int
ici_pdf_restore(void)
{
    return pdf_0(PDF_restore);
}

static int
ici_pdf_translate(void)
{
    return pdf_2f(PDF_translate);
}

static int
ici_pdf_scale(void)
{
    return pdf_2f(PDF_scale);
}

static int
ici_pdf_rotate(void)
{
    return pdf_1f(PDF_rotate);
}

static int
ici_pdf_skew(void)
{
    return pdf_2f(PDF_skew);
}

static int
ici_pdf_concat(void)
{
    return pdf_6f(PDF_concat);
}

static int
ici_pdf_setmatrix(void)
{
    return pdf_6f(PDF_setmatrix);
}

static int
ici_pdf_moveto(void)
{
    return pdf_2f(PDF_moveto);
}

static int
ici_pdf_lineto(void)
{
    return pdf_2f(PDF_lineto);
}

static int
ici_pdf_curveto(void)
{
    return pdf_6f(PDF_curveto);
}

static int
ici_pdf_circle(void)
{
    return pdf_3f(PDF_circle);
}

static int
ici_pdf_arc(void)
{
    return pdf_5f(PDF_arc);
}

static int
ici_pdf_arcn(void)
{
    return pdf_5f(PDF_arcn);
}

static int
ici_pdf_rect(void)
{
    return pdf_4f(PDF_rect);
}

static int
ici_pdf_closepath(void)
{
    return pdf_0(PDF_closepath);
}

static int
ici_pdf_stroke(void)
{
    return pdf_0(PDF_stroke);
}

static int
ici_pdf_closepath_stroke(void)
{
    return pdf_0(PDF_closepath_stroke);
}

static int
ici_pdf_fill(void)
{
    return pdf_0(PDF_fill);
}

static int
ici_pdf_fill_stroke(void)
{
    return pdf_0(PDF_fill_stroke);
}

static int
ici_pdf_closepath_fill_stroke(void)
{
    return pdf_0(PDF_closepath_fill_stroke);
}

static int
ici_pdf_endpath(void)
{
    return pdf_0(PDF_endpath);
}

static int
ici_pdf_clip(void)
{
    return pdf_0(PDF_clip);
}

static int
ici_pdf_setgray_fill(void)
{
    return pdf_1f(PDF_setgray_fill);
}

static int
ici_pdf_setgray_stroke(void)
{
    return pdf_1f(PDF_setgray_stroke);
}

static int
ici_pdf_setgray(void)
{
    return pdf_1f(PDF_setgray);
}

static int
ici_pdf_setrgbcolor_fill(void)
{
     return pdf_3f(PDF_setrgbcolor_fill);
}

static int
ici_pdf_setrgbcolor_stroke(void)
{
     return pdf_3f(PDF_setrgbcolor_stroke);
}

static int
ici_pdf_setrgbcolor(void)
{
     return pdf_3f(PDF_setrgbcolor);
}

static int
ici_pdf_makespotcolor(void)
{
    ici_handle_t *pdf;
    char *spotname;
    int len;

    if (ici_typecheck("hs", ICIS(PDF), &pdf, &spotname))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    len = strlen(spotname);
    return pdf_int_ret(PDF_makespotcolor((PDF *)pdf->h_ptr, spotname, len));
}

static int
ici_pdf_setcolor(void)
{
    ici_handle_t *pdf;
    char *fstype;
    char *colorspace;
    float c1, c2, c3, c4;

    if (ici_typecheck("hssnnnn", ICIS(PDF), &pdf, &fstype, &colorspace, &c1, &c2, &c3, &c4))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_setcolor((PDF *)pdf->h_ptr, fstype, colorspace, c1, c2, c3, c4);
    return pdf_null_ret();
}

static int
ici_pdf_begin_pattern(void)
{
    ici_handle_t *pdf;
    double w, h, x, y;
    long ptype;

    if (ici_typecheck("hnnnni", ICIS(PDF), &pdf, &w, &h, &x, &y, &ptype))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    return pdf_int_ret(PDF_begin_pattern((PDF *)pdf->h_ptr, w, h, x, y, ptype));
}

static int
ici_pdf_end_pattern(void)
{
    return pdf_0(PDF_end_pattern);
}

static int
ici_pdf_begin_template(void)
{
    return pdf_2f_i(PDF_begin_template);
}

static int
ici_pdf_end_template(void)
{
    return pdf_0(PDF_end_template);
}

static int
ici_pdf_place_image(void)
{
    ici_handle_t *pdf;
    int image;
    double x, y, s;

    if (ici_typecheck("hinnn", ICIS(PDF), &pdf, &image, &x, &y, &s))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_place_image((PDF *)pdf->h_ptr, image, x, y, s);
    return pdf_null_ret();
}

static int
ici_pdf_open_image_file(void)
{
    ici_handle_t *pdf;
    char *imagetype;
    char *filename;
    char *sparam;
    long iparam;

    if (ici_typecheck("hsssi", ICIS(PDF), &pdf, &imagetype, &filename, &sparam, &iparam))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    return pdf_int_ret(PDF_open_image_file((PDF *)pdf->h_ptr, imagetype, filename, sparam, iparam));
}

static int
ici_pdf_close_image(void)
{
    return pdf_1d(PDF_close_image);
}

static int
ici_pdf_add_thumbnail(void)
{
    return pdf_1d(PDF_add_thumbnail);
}

static int
ici_pdf_add_bookmark(void)
{
    ici_handle_t *pdf;
    char *text;
    long parent;
    long open;

    if (ici_typecheck("hsii", ICIS(PDF), &pdf, &text, &parent, &open))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    return pdf_int_ret(PDF_add_bookmark((PDF *)pdf->h_ptr, text, parent, open));
}

static int
ici_pdf_set_info(void)
{
    ici_handle_t *pdf;
    char *a, *b;

    if (ici_typecheck("hss", ICIS(PDF), &pdf, &a, &b))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_set_info((PDF *)pdf->h_ptr, a, b);
    return pdf_null_ret();
}

static int
ici_pdf_attach_file(void)
{
    ici_handle_t *pdf;
    double llx, lly, urx, ury;
    char *filename, *desc, *author, *mimetype, *icon;

    if (ici_typecheck("hnnnnsssss", ICIS(PDF), &pdf, &llx, &lly, &urx, &ury,
                        &filename, &desc, &author, &mimetype, &icon))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_attach_file((PDF *)pdf->h_ptr, llx, lly, urx, ury,
                        filename, desc, author, mimetype, icon);
    return pdf_null_ret();
}

static int
ici_pdf_add_note(void)
{
    ici_handle_t *pdf;
    double llx, lly, urx, ury;
    char *contents, *title, *icon;
    long open;

    if (ici_typecheck("hnnnnsssi", ICIS(PDF), &pdf, &llx, &lly, &urx, &ury,
                        &contents, &title, &icon, &open))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_add_note((PDF *)pdf->h_ptr, llx, lly, urx, ury, contents, title, icon, open);
    return pdf_null_ret();
}

static int
ici_pdf_add_pdflink(void)
{
    ici_handle_t *pdf;
    double llx, lly, urx, ury;
    char *filename;
    long page;
    char *dest;

    if (ici_typecheck("hnnnnsis", ICIS(PDF), &pdf, &llx, &lly, &urx, &ury,
                        &filename, &page, &dest))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_add_pdflink((PDF *)pdf->h_ptr, llx, lly, urx, ury, filename, page, dest);
    return pdf_null_ret();
}

static int
ici_pdf_add_launchlink(void)
{
    ici_handle_t *pdf;
    double llx, lly, urx, ury;
    char *filename;

    if (ici_typecheck("hnnnns", ICIS(PDF), &pdf, &llx, &lly, &urx, &ury, &filename))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_add_launchlink((PDF *)pdf->h_ptr, llx, lly, urx, ury, filename);
    return pdf_null_ret();
}

static int
ici_pdf_add_weblink(void)
{
    ici_handle_t *pdf;
    double llx, lly, urx, ury;
    char *url;

    if (ici_typecheck("hnnnns", ICIS(PDF), &pdf, &llx, &lly, &urx, &ury, &url))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_add_weblink((PDF *)pdf->h_ptr, llx, lly, urx, ury, url);
    return pdf_null_ret();
}

static int
ici_pdf_set_border_style(void)
{
    ici_handle_t *pdf;
    char *style;
    double width;

    if (ici_typecheck("hsn", ICIS(PDF), &pdf, &style, &width))
        return 1;
    if (check_if_deleted(pdf))
        return 1;
    PDF_set_border_style((PDF *)pdf->h_ptr, style, width);
    return pdf_null_ret();
}

static int
ici_pdf_set_border_color(void)
{
    return pdf_3f(PDF_set_border_color);
}

static int
ici_pdf_set_border_dash(void)
{
    return pdf_2f(PDF_set_border_dash);
}

static int
ici_pdf_get_buffer(void)
{
    ici_handle_t *pdf;
    long sz;

    if (args_0(&pdf))
        return 1;
    return pdf_str_ret((char *)PDF_get_buffer((PDF *)pdf->h_ptr, &sz));
}

static int
ici_pdf_set_parameter(void)
{
    return pdf_2s(PDF_set_parameter);
}

static int
ici_pdf_get_parameter(void)
{
    ici_handle_t *pdf;
    char *str;
    double modifier = 0.0;
    if (args_1s1fo(&pdf, &str, &modifier))
        return 1;
    return pdf_str_ret(PDF_get_parameter((PDF *)pdf->h_ptr, str, modifier));
}

static int
ici_pdf_get_value(void)
{
    ici_handle_t *pdf;
    char *str;
    double modifier = 0.0;
    if (args_1s1fo(&pdf, &str, &modifier))
        return 1;
    return pdf_float_ret(PDF_get_value((PDF *)pdf->h_ptr, str, modifier));
}

static int
ici_pdf_set_value(void)
{
    ici_handle_t *pdf;
    char *str;
    double value = 0.0;
    if (args_1s1fo(&pdf, &str, &value))
        return 1;
    PDF_set_value((PDF *)pdf->h_ptr, str, value);
    return pdf_null_ret();
}

static cfunc_t ici_pdf_cfuncs[] =
{
    {CF_OBJ, "get_majorversion", ici_pdf_get_majorversion},
    {CF_OBJ, "get_minorversion", ici_pdf_get_minorversion},
    {CF_OBJ, "boot", ici_pdf_boot},
    {CF_OBJ, "shutdown", ici_pdf_shutdown},
    {CF_OBJ, "new", ici_pdf_new},
    {CF_OBJ, "delete", ici_pdf_delete},
    {CF_OBJ, "open_file", ici_pdf_open_file},
    {CF_OBJ, "close", ici_pdf_close},
    {CF_OBJ, "begin_page", ici_pdf_begin_page},
    {CF_OBJ, "end_page", ici_pdf_end_page},
    {CF_OBJ, "findfont", ici_pdf_findfont},
    {CF_OBJ, "setfont", ici_pdf_setfont},
    {CF_OBJ, "show", ici_pdf_show},
    {CF_OBJ, "show_xy", ici_pdf_show_xy},
    {CF_OBJ, "continue_text", ici_pdf_continue_text},
    {CF_OBJ, "show_boxed", ici_pdf_show_boxed},
    {CF_OBJ, "set_text_matrix", ici_pdf_set_text_matrix},
    {CF_OBJ, "set_text_pos", ici_pdf_set_text_pos},
    {CF_OBJ, "string_width", ici_pdf_string_width},
    {CF_OBJ, "setdash", ici_pdf_setdash},
    {CF_OBJ, "setpolydash", ici_pdf_setpolydash},
    {CF_OBJ, "setflat", ici_pdf_setflat},
    {CF_OBJ, "setlinejoin", ici_pdf_setlinejoin},
    {CF_OBJ, "setlinecap", ici_pdf_setlinecap},
    {CF_OBJ, "setmiterlimit", ici_pdf_setmiterlimit},
    {CF_OBJ, "setlinewidth", ici_pdf_setlinewidth},
    {CF_OBJ, "initgraphics", ici_pdf_initgraphics},
    {CF_OBJ, "save", ici_pdf_save},
    {CF_OBJ, "restore", ici_pdf_restore},
    {CF_OBJ, "translate", ici_pdf_translate},
    {CF_OBJ, "scale", ici_pdf_scale},
    {CF_OBJ, "rotate", ici_pdf_rotate},
    {CF_OBJ, "skew", ici_pdf_skew},
    {CF_OBJ, "concat", ici_pdf_concat},
    {CF_OBJ, "setmatrix", ici_pdf_setmatrix},
    {CF_OBJ, "moveto", ici_pdf_moveto},
    {CF_OBJ, "lineto", ici_pdf_lineto},
    {CF_OBJ, "curveto", ici_pdf_curveto},
    {CF_OBJ, "circle", ici_pdf_circle},
    {CF_OBJ, "arc", ici_pdf_arc},
    {CF_OBJ, "arcn", ici_pdf_arcn},
    {CF_OBJ, "rect", ici_pdf_rect},
    {CF_OBJ, "closepath", ici_pdf_closepath},
    {CF_OBJ, "stroke", ici_pdf_stroke},
    {CF_OBJ, "closepath_stroke", ici_pdf_closepath_stroke},
    {CF_OBJ, "fill", ici_pdf_fill},
    {CF_OBJ, "fill_stroke", ici_pdf_fill_stroke},
    {CF_OBJ, "closepath_fill_stroke", ici_pdf_closepath_fill_stroke},
    {CF_OBJ, "endpath", ici_pdf_endpath},
    {CF_OBJ, "clip", ici_pdf_clip},
    {CF_OBJ, "setgray_fill", ici_pdf_setgray_fill},
    {CF_OBJ, "setgray_stroke", ici_pdf_setgray_stroke},
    {CF_OBJ, "setgray", ici_pdf_setgray},
    {CF_OBJ, "setrgbcolor_fill", ici_pdf_setrgbcolor_fill},
    {CF_OBJ, "setrgbcolor_stroke", ici_pdf_setrgbcolor_stroke},
    {CF_OBJ, "setrgbcolor", ici_pdf_setrgbcolor},
    {CF_OBJ, "makespotcolor", ici_pdf_makespotcolor},
    {CF_OBJ, "setcolor", ici_pdf_setcolor},
    {CF_OBJ, "begin_pattern", ici_pdf_begin_pattern},
    {CF_OBJ, "end_pattern", ici_pdf_end_pattern},
    {CF_OBJ, "begin_template", ici_pdf_begin_template},
    {CF_OBJ, "end_template", ici_pdf_end_template},
    {CF_OBJ, "place_image", ici_pdf_place_image},
    {CF_OBJ, "open_image_file", ici_pdf_open_image_file},
    {CF_OBJ, "close_image", ici_pdf_close_image},
    {CF_OBJ, "add_thumbnail", ici_pdf_add_thumbnail},
    {CF_OBJ, "add_bookmark", ici_pdf_add_bookmark},
    {CF_OBJ, "set_info", ici_pdf_set_info},
    {CF_OBJ, "attach_file", ici_pdf_attach_file},
    {CF_OBJ, "add_note", ici_pdf_add_note},
    {CF_OBJ, "add_pdflink", ici_pdf_add_pdflink},
    {CF_OBJ, "add_launchlink", ici_pdf_add_launchlink},
    {CF_OBJ, "add_weblink", ici_pdf_add_weblink},
    {CF_OBJ, "set_border_style", ici_pdf_set_border_style},
    {CF_OBJ, "set_border_color", ici_pdf_set_border_color},
    {CF_OBJ, "set_border_dash", ici_pdf_set_border_dash},
    {CF_OBJ, "get_buffer", ici_pdf_get_buffer},
    {CF_OBJ, "set_parameter", ici_pdf_set_parameter},
    {CF_OBJ, "get_parameter", ici_pdf_get_parameter},
    {CF_OBJ, "get_value", ici_pdf_get_value},
    {CF_OBJ, "set_value", ici_pdf_set_value},
    {CF_OBJ}
};

object_t *
ici_pdf_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "pdf"))
        return NULL;
    if (init_ici_str())
        return NULL;
    return objof(ici_module_new(ici_pdf_cfuncs));
}

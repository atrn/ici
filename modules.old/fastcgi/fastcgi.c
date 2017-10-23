#define ICI_NO_OLD_NAMES

#include <ici.h>
#include "icistr.h"
#include <icistr-setup.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcgiapp.h>

/******************************************************************/

#ifdef DEBUG
static void
LOG(const char *fmt, ...)
{
    FILE *f = fopen("/tmp/ici4fastcgi.log", "a");
    if (f != NULL)
    {
	va_list va;
	va_start(va, fmt);
	vfprintf(f, fmt, va);
	fprintf(f, "\n");
	va_end(va);
	fclose(f);
    }
}
#endif

static int
stream_getch(FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_getch %p", stream);
#endif
    return FCGX_GetChar(stream);
}

static int
stream_ungetch(int ch, FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_ungetch %p", stream);
#endif
    return FCGX_UnGetChar(ch, stream);
}

static int
stream_putch(int ch, FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_putch %p", stream);
#endif
    return 0;
}

static int
stream_flush(FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_flush %p", stream);
#endif
    return FCGX_FFlush(stream);
}

static int
stream_close(FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_close %p", stream);
#endif
    return FCGX_FClose(stream);
}

static long
stream_seek()
{
#ifdef DEBUG
    LOG("stream_seek");
#endif
    return EOF;
}

static int
stream_eof(FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream_eof %p", stream);
#endif
    return FCGX_HasSeenEOF(stream);
}

static int
stream_write(const char *buffer, int n, FCGX_Stream *stream)
{
    int rc;

#ifdef DEBUG
    LOG("stream_write %p buffer %p size %d", stream, buffer, n);
#endif
    rc = FCGX_PutStr(buffer, n, stream);
#ifdef DEBUG
    LOG("stream_write returned %d", rc);
#endif
    return rc;
}

static ici_ftype_t stream_ftype =
{
    0,
    stream_getch,
    stream_ungetch,
    stream_putch,
    stream_flush,
    stream_close,
    stream_seek,
    stream_eof,
    stream_write
};

/******************************************************************/

#ifdef DEBUG
static void
dump_env(FCGX_ParamArray e)
{
    while (*e != NULL)
    {
        LOG("%s", *e);
        ++e;
    }
}
#endif

static int
fa_env(ici_handle_t *h, ici_obj_t *k, ici_obj_t *setv, ici_obj_t **retv)
{
    char *value;

#ifdef DEBUG
    dump_env(h->h_ptr);
#endif

    if (setv != NULL)
    {
        return ici_set_error("attempt to set fastcgi environment");
    }
    if (!ici_isstring(k))
    {
        return ici_set_error("attempt to fetch from fastcgi environment with a %s key", ici_typeof(k)->t_name);
    }
#ifdef DEBUG
    LOG("fa_env: %p \"%s\"", h->h_ptr, ici_stringof(k)->s_chars);
#endif
    if ((value = FCGX_GetParam(ici_stringof(k)->s_chars, h->h_ptr)) == NULL)
    {
        return ici_set_error("key \"%s\" not found in fastcgi environment", ici_stringof(k)->s_chars);
    }
    if ((*retv = ici_objof(ici_str_new_nul_term(value))) == NULL)
    {
        return 1;
    }
    return 0;
}

/******************************************************************/

static ici_obj_t *
handle(ici_str_t *name, void *p, int (*fa)(ici_handle_t *, ici_obj_t *, ici_obj_t *, ici_obj_t **))
{
    ici_handle_t *h = ici_handle_new(p, name, NULL);
#ifdef DEBUG
    LOG("%s %p", name->s_chars, p);
#endif
    if (h != NULL)
    {
        h->h_general_intf = fa;
    }
    return ici_objof(h);
}

static ici_obj_t *
stream(ici_str_t *name, FCGX_Stream *stream)
{
#ifdef DEBUG
    LOG("stream %s %p", name->s_chars, stream);
#endif
    return ici_objof(ici_file_new(stream, &stream_ftype, name, NULL));
}

/******************************************************************/

static int
f_is_cgi(void)
{
    if (ici_typecheck(""))
        return 1;
    return ici_int_ret(FCGX_IsCGI());
}

static int
f_init(void)
{
    if (ici_typecheck(""))
        return 1;
    return ici_int_ret(FCGX_Init());
}

static int
f_open_socket(void)
{
    char *path;
    long backlog;
    int skt;

    if (ici_typecheck("si", &path, &backlog))
    {
        return 1;
    }
    skt = FCGX_OpenSocket(path, backlog);
    if (skt == -1)
    {
        return ici_set_error("%s", strerror(errno));
    }
    return ici_int_ret(skt);
}

static int
assign(ici_struct_t *d, ici_str_t *k, ici_obj_t *v)
{
    if (v == NULL)
        return 1;
    if (ici_assign(ici_objof(d), ici_objof(k), v))
    {
        ici_decref(v);
        return 1;
    }
    ici_decref(v);
    return 0;
}

static int
f_accept(void)
{
    FCGX_Stream         *in;
    FCGX_Stream         *out;
    FCGX_Stream         *err;
    FCGX_ParamArray     env;
    ici_struct_t        *d;

    if (ici_typecheck(""))
        return 1;
    if ((d = ici_struct_new()) == NULL)
    {
	return 1;
    }
    if (FCGX_Accept(&in, &out, &err, &env) < 0)
    {
	ici_decref(d);
        return ici_null_ret();
    }
    if (assign(d, ICIS(in), stream(ICIS(in), in)))
	goto fail;
    if (assign(d, ICIS(out), stream(ICIS(out), out)))
	goto fail;
    if (assign(d, ICIS(err), stream(ICIS(err), err)))
	goto fail;
    if (assign(d, ICIS(env), handle(ICIS(env), env, fa_env)))
	goto fail;
    return ici_ret_with_decref(ici_objof(d));

fail:
    ici_decref(d);
    return 1;
}

static int
f_get_param(void)
{
    char                *name;
    ici_handle_t        *env;
    char                *param;

    if (ici_typecheck("sh", &name, ICIS(env), &env))
        return 1;
    if ((param = FCGX_GetParam(name, env->h_ptr)) == NULL)
        return ici_null_ret();
    return ici_str_ret(param);
}

static int
f_finish(void)
{
    if (ici_typecheck(""))
        return 1;
    FCGX_Finish();
    return ici_null_ret();
}

static void
free_request(ici_handle_t *h)
{
    ici_tfree(h->h_ptr, FCGX_Request);
}

static int
f_init_request(void)
{
    long                sock;
    long                flags;
    ici_handle_t        *h;
    FCGX_Request        *r;

    if (ici_typecheck("ii", &sock, &flags))
    {
        if (ici_typecheck("i", &sock))
        {
            return 1;
        }
        flags = 0;
    }
    if ((r = ici_talloc(FCGX_Request)) == NULL)
    {
        return 1;
    }
    if ((h = ici_handle_new(r, ICIS(FCGX_Request), NULL)) == NULL)
    {
        ici_tfree(r, FCGX_Request);
        return 1;
    }
    h->h_pre_free = free_request;
    if (FCGX_InitRequest(r, sock, flags) != 0)
    {
        return ici_set_error("FCGX_InitRequest failed");
    }
    return ici_ret_with_decref(ici_objof(h));
}

static int
f_accept_r(void)
{
    ici_handle_t *h;

    if (ici_typecheck("h", ICIS(FCGX_Request), &h))
    {
        return 1;
    }
    return ici_int_ret(FCGX_Accept_r(h->h_ptr));
}

static int
f_finish_r(void)
{
    ici_handle_t        *h;

    if (ici_typecheck("h", ICIS(FCGX_Request), &h))
    {
        return 1;
    }
    FCGX_Finish_r(h->h_ptr);
    return ici_null_ret();
}

static int
f_free(void)
{
    ici_handle_t        *h;
    long                close;

    if (ici_typecheck("hi", ICIS(FCGX_Request), &h, &close))
    {
        if (ici_typecheck("h", ICIS(FCGX_Request), &h))
        {
            return 1;
        }
        close = 1;
    }
    FCGX_Free(h->h_ptr, close);
    return ici_null_ret();
}

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ,        "is_cgi",       f_is_cgi},
    {ICI_CF_OBJ,        "init",         f_init},
    {ICI_CF_OBJ,        "open_socket",  f_open_socket},
    {ICI_CF_OBJ,        "init_request", f_init_request},
    {ICI_CF_OBJ,        "accept",       f_accept},
    {ICI_CF_OBJ,        "accept_r",     f_accept_r},
    {ICI_CF_OBJ,        "get_param",    f_get_param},
    {ICI_CF_OBJ,        "finish",       f_finish},
    {ICI_CF_OBJ,        "finish_r",     f_finish_r},
    {ICI_CF_OBJ,        "free",         f_free},
    {ICI_CF_OBJ}
};

ici_obj_t *
ici_fastcgi_library_init(void)
{
    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "str"))
        return NULL;
    if (init_ici_str())
        return NULL;
    return ici_objof(ici_module_new(cfuncs));
}

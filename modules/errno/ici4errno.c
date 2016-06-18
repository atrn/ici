#include <ici.h>
#include <errno.h>

/*
 * System error codes
 *
 * This --intro-- is part of the --ici-errno-- documentation.
 */

static ici_cfunc_t cfuncs[] =
{
    {ICI_CF_OBJ}
};

#define F(N) { N, #N }

static struct errno_table
{
    int  code;
    char *name;
}
errnos[] =
{
    F(EPERM),
    F(ENOENT),
    F(ESRCH),
    F(EINTR),
    F(EIO),
    F(ENXIO),
    F(E2BIG),
    F(ENOEXEC),
    F(EBADF),
    F(ECHILD),
    F(EDEADLK),
    F(ENOMEM),
    F(EACCES),
    F(EFAULT),
    F(ENOTBLK),
    F(EBUSY),
    F(EEXIST),
    F(EXDEV),
    F(ENODEV),
    F(ENOTDIR),
    F(EISDIR),
    F(EINVAL),
    F(ENFILE),
    F(EMFILE),
    F(ENOTTY),
    F(ETXTBSY),
    F(EFBIG),
    F(ENOSPC),
    F(ESPIPE),
    F(EROFS),
    F(EMLINK),
    F(EPIPE),
    F(EDOM),
    F(ERANGE),
    F(EAGAIN),
    F(EWOULDBLOCK),
    F(EINPROGRESS),
    F(EALREADY),
    F(ENOTSOCK),
    F(EDESTADDRREQ),
    F(EMSGSIZE),
    F(EPROTOTYPE),
    F(ENOPROTOOPT),
    F(EPROTONOSUPPORT),
    F(ESOCKTNOSUPPORT),
    F(EOPNOTSUPP),
    F(ENOTSUP),
    F(EPFNOSUPPORT),
    F(EAFNOSUPPORT),
    F(EADDRINUSE),
    F(EADDRNOTAVAIL),
    F(ENETDOWN),
    F(ENETUNREACH),
    F(ENETRESET),
    F(ECONNABORTED),
    F(ECONNRESET),
    F(ENOBUFS),
    F(EISCONN),
    F(ENOTCONN),
    F(ESHUTDOWN),
    F(ETOOMANYREFS),
    F(ETIMEDOUT),
    F(ECONNREFUSED),
    F(ELOOP),
    F(ENAMETOOLONG),
    F(EHOSTDOWN),
    F(EHOSTUNREACH),
    F(ENOTEMPTY),
#ifdef EPROCLIM
    F(EPROCLIM),
#endif
    F(EUSERS),
    F(EDQUOT),
    F(ESTALE),
    F(EREMOTE),
#ifdef EBADRPC
    F(EBADRPC),
#endif
#ifdef ERPCMISMATCH
    F(ERPCMISMATCH),
#endif
#ifdef EPROGUNAVAIL
    F(EPROGUNAVAIL),
#endif
#ifdef EPROGMISMATCH
    F(EPROGMISMATCH),
#endif
#ifdef EPROCUNAVAIL
    F(EPROCUNAVAIL),
#endif
    F(ENOLCK),
    F(ENOSYS),
#ifdef EFTYPE
    F(EFTYPE),
#endif
#ifdef EAUTH
    F(EAUTH),
#endif
#ifdef ENEEDAUTH
    F(ENEEDAUTH),
#endif
    F(EIDRM),
    F(ENOMSG),
    F(EOVERFLOW),
    F(ECANCELED),
    F(EILSEQ),
#ifdef ENOATTR
    F(ENOATTR),
#endif
#ifdef EDOOFUS
    F(EDOOFUS),
#endif
    F(EBADMSG),
    F(EMULTIHOP),
    F(ENOLINK),
    F(EPROTO),
#ifdef ELAST
    F(ELAST),
#endif
    {0, 0}
};

ici_obj_t *
ici_errno_library_init(void)
{
    ici_objwsup_t *module;
    struct errno_table *p;

    if (ici_interface_check(ICI_VER, ICI_BACK_COMPAT_VER, "errno"))
        return NULL;
    if ((module = ici_module_new(cfuncs)) == NULL)
        return NULL;
    for (p = errnos; p->code != 0; ++p)
    {
        ici_str_t *s = ici_str_new_nul_term(p->name);
        if (s == NULL)
            goto fail;
        ici_set_val(module, s, 'i', &p->code);
        ici_decref(s);
    }
    return ici_objof(module);

fail:
    ici_decref(module);
    return NULL;
}

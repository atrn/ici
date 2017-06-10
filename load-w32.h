// -*- mode:c++ -*-

#ifndef ICI_LOAD_W32_H
#define ICI_LOAD_W32_H

#include <windows.h>

namespace ici
{

typedef void    *dll_t;

#define valid_dll(dll)  ((dll) != NULL)

static dll_t
dlopen(const char *name, int mode)
{
    return LoadLibrary(name);
}

static void *
dlsym(dll_t hinst, const char *name)
{
    return GetProcAddress(hinst, name);
}

static char *
dlerror()
{
    static char     msg[80];

    FormatMessage
    (
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        msg,
        sizeof msg,
        NULL
    );
    return msg;
}

static void
dlclose(dll_t hinst)
{
}

} // namespace ici

#endif /* ICI_LOAD_W32_H */

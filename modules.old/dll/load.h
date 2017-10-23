#if defined(_WIN32)

#   include <windows.h>

    typedef void    *dll_t;

#   define valid_dll(dll)  ((dll) != NULL)

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
    dlerror(void)
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
        FreeLibrary(hinst);
    }


#else /* Unix */

#   include <sys/types.h>
#   include <dlfcn.h>
#   include <unistd.h>

    typedef void *dll_t;
#   define valid_dll(dll)  ((dll) != NULL)


#endif /* _WIN32 vs UNIX */

#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#include "fwd.h"
#include <windows.h>
#include <io.h>

static char             **argv;
static int              argc;

static int
determine_argv(HINSTANCE inst, char *cmd_line)
{
    char                *p;
    int                 i;
    int                 n;
    char                *argv0;
    char                *argv1;
    char                fname[FILENAME_MAX];

    if (!GetModuleFileName(inst, fname, sizeof fname))
        goto fail;
    if ((argv0 = _strdup(fname)) == nullptr)
        goto fail;
    argv1 = nullptr;
    if
    (
        (p = strchr(fname, '.')) != nullptr
        &&
        stricmp(p, ".exe") == 0
        &&
        (strcpy(p, ".ici"), access(fname, 4) == 0)
        &&
        (argv1 = _strdup(fname)) == nullptr
    )
        goto fail;

    if ((p = cmd_line) == nullptr)
    {
        i = 0;
    }
    else
    {
        for (i = 1; (p = strchr(p, ' ')) != nullptr; ++p)
            ++i;
    }
    n = 1                   /* argv0 */
        + (argv1 != nullptr)   /* argv1 */
        + i
        + 1;                /* nullptr on end. */
    if ((argv = (char **)malloc(n * sizeof(char *))) == nullptr)
        goto fail;

    i = 0;
    argv[i++] = argv0;
    if (argv1 != nullptr)
        argv[i++] = argv1;
    if ((p = cmd_line) != nullptr)
    {
        argv[i++] = p;
        while ((p = strchr(p, ' ')) != nullptr)
        {
            *p++ = '\0';
            argv[i++] = p;
        }
    }
    argv[i] = nullptr;
    argc = i;
    return 0;

fail:
    return 1;
}

int WINAPI
WinMain(HINSTANCE inst, HINSTANCE prev_inst, char *cmd_line, int cmd_show)
{
    freopen("stderr.txt", "w", stderr);
    freopen("stdout.txt", "w", stdout);

    if (determine_argv(inst, cmd_line))
    {
        MessageBox(nullptr, "Ran out of memory.", "ICI", MB_OK);
        return EXIT_FAILURE;
    }
    if (ici_main(argc, argv))
    {
        MessageBox(nullptr, ici_error, "ICI", MB_OK);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


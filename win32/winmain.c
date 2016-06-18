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
    if ((argv0 = _strdup(fname)) == NULL)
        goto fail;
    argv1 = NULL;
    if
    (
        (p = strchr(fname, '.')) != NULL
        &&
        stricmp(p, ".exe") == 0
        &&
        (strcpy(p, ".ici"), access(fname, 4) == 0)
        &&
        (argv1 = _strdup(fname)) == NULL
    )
        goto fail;

    if ((p = cmd_line) == NULL)
    {
        i = 0;
    }
    else
    {
        for (i = 1; (p = strchr(p, ' ')) != NULL; ++p)
            ++i;
    }
    n = 1                   /* argv0 */
        + (argv1 != NULL)   /* argv1 */
        + i
        + 1;                /* NULL on end. */
    if ((argv = (char **)malloc(n * sizeof(char *))) == NULL)
        goto fail;

    i = 0;
    argv[i++] = argv0;
    if (argv1 != NULL)
        argv[i++] = argv1;
    if ((p = cmd_line) != NULL)
    {
        argv[i++] = p;
        while ((p = strchr(p, ' ')) != NULL)
        {
            *p++ = '\0';
            argv[i++] = p;
        }
    }
    argv[i] = NULL;
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
        MessageBox(NULL, "Ran out of memory.", "ICI", MB_OK);
        return EXIT_FAILURE;
    }
    if (ici_main(argc, argv))
    {
        MessageBox(NULL, ici_error, "ICI", MB_OK);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


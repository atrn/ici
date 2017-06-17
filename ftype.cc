#define ICI_CORE
#include "ftype.h"

namespace ici
{

int ftype::ft_getch(void *)
{
    return -1;
}

int ftype::ft_ungetch(int, void *)
{
    return -1;
}

int ftype::ft_flush(void *)
{
    return 0;
}

int ftype::ft_close(void *)
{
    return 0;
}

int long ftype::ft_seek(void *, long, int)
{
    return -1l;
}

int ftype::ft_eof(void *)
{
    return 0;
}

int ftype::ft_write(const void *, long, void *)
{
    return 0;
}

int ftype::ft_fileno(void *)
{
    return -1;
}

int ftype::ft_setvbuf(void *, char *, int, size_t)
{
    return -1;
}

//================================================================

stdio_ftype::stdio_ftype()
    : ftype(FT_NOMUTEX)
{
}

int stdio_ftype::ft_getch(void *file)
{
    return fgetc((FILE *)file);
}

int stdio_ftype::ft_ungetch(int c, void *file)
{
    return ungetc(c, (FILE *)file);
}

int stdio_ftype::ft_flush(void *file)
{
    return fflush((FILE *)file);
}

int stdio_ftype::ft_close(void *file)
{
    return fclose((FILE *)file);
}

long stdio_ftype::ft_seek(void *file, long offset, int whence)
{
    if (fseek((FILE *)file, offset, whence) == -1)
    {
        ici_set_error("seek failed");
        return -1;
    }
    return ftell((FILE *)file);
}

int stdio_ftype::ft_eof(void *file)
{
    return feof((FILE *)file);
}

int stdio_ftype::ft_write(const void *buf, long n, void *file)
{
    return fwrite(buf, 1, (size_t)n, (FILE *)file);
}

int stdio_ftype::ft_fileno(void *f)
{
    return fileno((FILE *)f);
}

int stdio_ftype::ft_setvbuf(void *f, char *buf, int typ, size_t size)
{
    return setvbuf((FILE *)f, buf, typ, size);
}

//================================================================

int popen_ftype::ft_close(void *file)
{
    return pclose((FILE *)file);
}

ici_ftype_t *ici_stdio_ftype = instance_of<stdio_ftype>();
ici_ftype_t *ici_popen_ftype = instance_of<popen_ftype>();

}

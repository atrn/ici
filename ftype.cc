#define ICI_CORE
#include "ftype.h"

namespace ici {

int ftype::getch(void *) {
    return -1;
}

int ftype::ungetch(int, void *) {
    return -1;
}

int ftype::flush(void *) {
    return 0;
}

int ftype::close(void *) {
    return 0;
}

int long ftype::seek(void *, long, int) {
    return -1l;
}

int ftype::eof(void *) {
    return 0;
}

int ftype::write(const void *, long, void *) {
    return 0;
}

int ftype::fileno(void *) {
    return -1;
}

int ftype::setvbuf(void *, char *, int, size_t) {
    return -1;
}

//================================================================

stdio_ftype::stdio_ftype() : ftype(nomutex) {
}

int stdio_ftype::getch(void *file) {
    return ::fgetc((FILE *)file);
}

int stdio_ftype::ungetch(int c, void *file) {
    return ::ungetc(c, (FILE *)file);
}

int stdio_ftype::flush(void *file) {
    return ::fflush((FILE *)file);
}

int stdio_ftype::close(void *file) {
    return ::fclose((FILE *)file);
}

long stdio_ftype::seek(void *file, long offset, int whence) {
    if (::fseek((FILE *)file, offset, whence) == -1) {
        set_error("seek failed");
        return -1;
    }
    return ::ftell((FILE *)file);
}

int stdio_ftype::eof(void *file) {
    return ::feof((FILE *)file);
}

int stdio_ftype::write(const void *buf, long n, void *file) {
    return ::fwrite(buf, 1, (size_t)n, (FILE *)file);
}

int stdio_ftype::fileno(void *f) {
    return ::fileno((FILE *)f);
}

int stdio_ftype::setvbuf(void *f, char *buf, int typ, size_t size) {
    return ::setvbuf((FILE *)f, buf, typ, size);
}

//================================================================

int popen_ftype::close(void *file) {
    return ::pclose((FILE *)file);
}

//================================================================

ftype *stdio_ftype = ptr_to_instance_of<class stdio_ftype>();
ftype *popen_ftype = ptr_to_instance_of<class popen_ftype>();

} // namespace ici

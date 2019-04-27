#include <ici.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "util.h"

/*
 * mem = getdata(string)
 *
 * Returns a, byte-accessed, mem object containing the data from the
 * named file.
 */
int f_util_getdata()
{
    char *filename;
    
    if (ici::typecheck("s", &filename))
    {
        return 1;
    }
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        return ici::get_last_errno("open", filename);
    }

    struct must_close
    {
        must_close(int fd) : fd(fd) {}
        ~must_close() { close(fd); }
        const int fd;
    }
    opened_file_descriptor(fd);

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1)
    {
        return ici::get_last_errno("fstat", filename);
    }
    
    char *p = static_cast<char *>(ici::ici_alloc(statbuf.st_size));
    if (!p)
    {
        return 1;
    }

    ici::ref<ici::mem> m = ici::new_mem(p, statbuf.st_size, 1, ici::ici_free);
    if (!m)
    {
        ici::ici_free(p);
        return 1;
    }

    for (int n, nr = 0; nr < statbuf.st_size; nr += n)
    {
        n = read(fd, p + nr, statbuf.st_size - nr);
        switch (n)
        {
        case 0:
            goto done;
        case -1:
            return ici::get_last_errno("read", filename);
        }
    }

done:
    return ici::ret_with_decref(m.release());
}

#ifndef ICI_DLFCN_SIMPLE_H
#define ICI_DLFCN_SIMPLE_H

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100

void *dlopen(const char *path, int mode);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle);
const char *dlerror(void);

#endif /* #ifndef ICI_DLFCN_SIMPLE_H */

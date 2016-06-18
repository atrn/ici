#ifndef ICI_LOAD_BEOS_H
#define ICI_LOAD_BEOS_H

#ifndef __BEOS__
# error Cannot include "load-beos.h" on non BeOS platforms
#endif

#include <kernel/image.h>

typedef image_id        dll_t ;
#define valid_dll(dll)  ((dll) > 0)

/*
 * Keep track of the error, since we can't use errno to get the current
 * image load error
 */
static status_t beos_image_load_ici_error = 0 ;

static dll_t
dlopen(const char *name, int mode)
{
    char path[256];
    dll_t image;

    if (name[0] == '.')
    {
        getcwd(path, sizeof(path));
        strcat(path, name+1);
        name = path;
    }
    else if (name[0] != '/')
    {
        getcwd(path, sizeof(path));
        strcat(path, "/");
        strcat(path, name);
        name = path;
    }
    image = load_add_on(name);
    if (image < 0)
    {
        beos_image_load_ici_error = image;
        return 0;
    }
    beos_image_load_ici_error = 0;
    return image;
}

static void *
dlsym(dll_t image, const char *name)
{
    void        *addr;

    beos_image_load_ici_error = get_image_symbol(image, name, B_SYMBOL_TYPE_ANY, &addr);
    if (beos_image_load_error != B_OK)
    {
        return NULL;
    }
    return addr;
}

static char *
dlerror(void)
{
    return strerror(beos_image_load_error);
}

static void
dlclose(dll_t image)
{
    unload_add_on(image);
}

/************************
 The function below is not called by ICI, but if provided should ICI require
   more advanced dynamic loading at a future date.
*************************/

#if 0
typedef struct _dll_info
{
        dll_t           lib_id ;
        long            lib_load_count ;
        long            lib_init_count ;
        void            (*init_routine)( ) ;
        void            (*term_routine)( ) ;
        long            on_device ;
        long long       inode ;
        char            path[MAXPATHLEN] ;
        void  *         text_segment ;
        void  *         data_segment ;
        long            text_size ;
        long            data_size ;
}       dll_info ;

void dlinfo( dll_t image, dll_info * d_info )
{
        image_info i_info ;
        get_image_info( image, &i_info ) ;

        d_info->lib_id = i_info.id ;
        d_info->lib_load_count = i_info.sequence ;
        d_info->lib_init_count = i_info.init_order ;

        d_info->init_routine = i_info.init_routine ;
        d_info->term_routine = i_info.term_routine ;

        d_info->on_device = i_info.device ;
        d_info->inode = i_info.node ;

        strcpy( d_info->path , i_info.name ) ;

        d_info->text_segment = i_info.text ;
        d_info->data_segment = i_info.data ;

        d_info->text_size = i_info.text_size ;
        d_info->data_size = i_info.data_size ;
}
#endif   /* 0 */

#endif  /* ICI_LOAD_BEOS_H */

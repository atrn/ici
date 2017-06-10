#ifndef ICI_SEMAPHORE_H
#define ICI_SEMAPHORE_H

#include <fwd.h>

#ifdef ICI_USE_POSIX_THREADS

#include <pthread.h>

namespace ici
{
    
typedef struct
{
    unsigned int    sem_count;
    unsigned long   sem_nwaiters;
    pthread_mutex_t sem_mutex;
    pthread_cond_t  sem_condvar;
}
ici_sem_t;

int ici_sem_init(ici_sem_t *sem, int pshared, unsigned int count);
int ici_sem_destroy(ici_sem_t *sem);
int ici_sem_wait(ici_sem_t *sem);
int ici_sem_post(ici_sem_t *sem);

#endif

} // namespace ici

#endif /* #ifndef ICI_SEMAPHORE_H */

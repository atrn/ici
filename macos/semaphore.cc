/*
 * Counting semapores implemented with pthreads condition variables.
 */

#include <semaphore.h>

namespace ici
{

#ifdef ICI_USE_POSIX_THREADS

int
ici_sem_init(ici_sem_t *sem, int pshared, unsigned int count)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t condvar_attr;

    (void)pshared;

    if (pthread_mutexattr_init(&mutex_attr) != 0)
        return -1;
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&sem->sem_mutex, &mutex_attr) != 0)
    {
        pthread_mutexattr_destroy(&mutex_attr);
        return -1;
    }
    pthread_mutexattr_destroy(&mutex_attr);
    if (pthread_condattr_init(&condvar_attr) != 0)
    {
        pthread_mutexattr_destroy(&mutex_attr);
        return -1;
    }
    if (pthread_cond_init(&sem->sem_condvar, &condvar_attr) != 0)
    {
        pthread_condattr_destroy(&condvar_attr);
        pthread_mutex_destroy(&sem->sem_mutex);
        return -1;
    }
    pthread_condattr_destroy(&condvar_attr);
    sem->sem_count = count;
    sem->sem_nwaiters = 0;
    return 0;
}

int
ici_sem_destroy(ici_sem_t *sem)
{
    pthread_cond_destroy(&sem->sem_condvar);
    pthread_mutex_destroy(&sem->sem_mutex);
    return 0;
}

int
ici_sem_wait(ici_sem_t *sem)
{
    if (pthread_mutex_lock(&sem->sem_mutex) != 0)
        return -1;
    ++sem->sem_nwaiters;
    while (sem->sem_count == 0)
    {
        if (pthread_cond_wait(&sem->sem_condvar, &sem->sem_mutex) != 0)
        {
            (void)pthread_mutex_unlock(&sem->sem_mutex);
            return -1;
        }
    }
    --sem->sem_nwaiters;
    --sem->sem_count;
    return pthread_mutex_unlock(&sem->sem_mutex);
}

int
ici_sem_post(ici_sem_t *sem)
{
    if (pthread_mutex_lock(&sem->sem_mutex) != 0)
        return -1;
    if (sem->sem_nwaiters > 0)
    {
        if (pthread_cond_signal(&sem->sem_condvar) != 0)
        {
            (void)pthread_mutex_unlock(&sem->sem_mutex);
            return -1;
        }
    }
    ++sem->sem_count;
    return pthread_mutex_unlock(&sem->sem_mutex);
}

#endif /* ICI_USE_POSIX_THREADS */

} // namespace ici

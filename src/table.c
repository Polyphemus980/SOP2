#include "table.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include "macros.h"

table_t *table_init(int *player_idx)
{
    int shmem_fd;
    /* open semaphore for synchronization of shared memory initialization*/
    sem_t *shmem_init_sem;
    shmem_init_sem = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (shmem_init_sem == SEM_FAILED)
        ERR("sem_open");

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        ERR("clock_gettime");

    ts.tv_sec += 10;
    int s = 0;
    while ((s = sem_timedwait(shmem_init_sem, &ts)) == -1 && errno == EINTR)
        continue; /* Restart if interrupted by handler */

    /* Check what happened */
    if (s == -1)
    {
        if (errno == ETIMEDOUT)
            ERR("sem_timedwait() timed out\n");
        else
            ERR("sem_timedwait");
    }

    /* Try to create shared memory */
    int initialize = 1;
    shmem_fd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shmem_fd == -1)
    {
        if (errno != EEXIST)
        {
            sem_post(shmem_init_sem);
            ERR("shm_open");
        }

        /* Open existing shmem */
        shmem_fd = shm_open(SHMEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
        if (shmem_fd == -1)
        {
            sem_post(shmem_init_sem);
            ERR("shm_open");
        }
        initialize = 0;
    }

    if (initialize && ftruncate(shmem_fd, sizeof(table_t)) == -1)
    {
        // Destroy to prevent partially created shmem
        shm_unlink(SHMEM_NAME);
        sem_post(shmem_init_sem);
        ERR("ftruncate");
    }

    table_t *shmem = mmap(NULL, sizeof(table_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (shmem == MAP_FAILED)
    {
        if (initialize)  // Destroy to prevent partially created shmem
            shm_unlink(SHMEM_NAME);
        sem_post(shmem_init_sem);
        ERR("mmap");
    }

    close(shmem_fd);

    if (initialize)
    {
        errno = 0;

        pthread_barrierattr_t attr;
        pthread_barrierattr_init(&attr);
        pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_barrier_init(&shmem->players_barrier, &attr, PLAYERS_COUNT);

        pthread_mutexattr_t attr2;
        pthread_mutexattr_init(&attr2);
        pthread_mutexattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shmem->players_counter_lock, &attr2);
        pthread_mutex_init(&shmem->placed_lock, &attr2);

        pthread_condattr_t attr3;
        pthread_condattr_init(&attr3);
        pthread_condattr_setpshared(&attr3, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shmem->placed_cond, &attr3);

        for (int i = 0; i < CARDS_COUNT; ++i)
            shmem->cards[i] = i;

        for (int i = 0; i < PLAYERS_COUNT; ++i)
            shmem->trick[i] = INVALID_CARD;

        shmem->players_counter = 1;
        *player_idx = 0;
        for (int i = 0; i < 10; ++i)
            shuffle(shmem->cards, CARDS_COUNT);

        if (errno)  // Something went wrong => destroying shmem
        {
            // Destroy to prevent partially created shmem
            shm_unlink(SHMEM_NAME);
            sem_post(shmem_init_sem);
            ERR("shmem init");
        }
    }
    else
    {
        // Lock counter, because someone can be leaving at the moment
        pthread_mutex_lock(&shmem->players_counter_lock);
        *player_idx = shmem->players_counter++;
        pthread_mutex_unlock(&shmem->players_counter_lock);
    }
    sem_post(shmem_init_sem);

    return shmem;
}

void table_close(table_t *shmem)
{
    munmap(shmem, sizeof(table_t));
}

void table_destroy(table_t *shmem)
{
    pthread_cond_destroy(&shmem->placed_cond);
    pthread_mutex_destroy(&shmem->players_counter_lock);
    pthread_mutex_destroy(&shmem->placed_lock);
    pthread_barrier_destroy(&shmem->players_barrier);

    table_close(shmem);

    if (shm_unlink(SHMEM_NAME))
        ERR("shm_unlink");
    if (sem_unlink(SHMEM_SEMAPHORE_NAME))
        ERR("sem_unlink");
}

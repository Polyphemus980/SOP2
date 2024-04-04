#pragma once
#include <pthread.h>
#include "cards.h"

#define SHMEM_SEMAPHORE_NAME "/sop-shmem-sem"
#define SHMEM_NAME "/sop-shmem"

/**
 *
 */
typedef struct table
{
    /**
     *
     */
    pthread_barrier_t players_barrier;
    /**
     *
     */
    pthread_mutex_t players_counter_lock;
    /**
     *
     */
    int players_counter;

    /**
     *
     */
    pthread_cond_t placed_cond;
    /**
     *
     */
    pthread_mutex_t placed_lock;

    /**
     *
     */
    card_t trick[PLAYERS_COUNT];
    /**
     *
     */
    card_t cards[CARDS_COUNT];
} table_t;

/**
 *
 * @param player_idx
 * @return
 */
table_t* table_init( int* player_idx);

/**
 *
 * @param shmem
 * @param shmem_fd
 */
void table_close(table_t* shmem);

/**
 *
 * @param shmem
 * @param shmem_fd
 */
void table_destroy(table_t* shmem);

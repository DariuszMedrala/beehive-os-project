#ifndef QUEEN_H
#define QUEEN_H

#include "common.h"

/**
 * The QueenArgs struct contains all the parameters required by the queen process.
 * The queen is responsible for laying eggs at regular intervals and adding new bees to the hive.
 */
typedef struct {
    int T_k;       ///< Time interval (in seconds) between each egg-laying cycle.
    int eggsCount; ///< Number of eggs laid by the queen in one cycle.
    HiveData* hive; ///< Pointer to the shared memory structure representing the hive state.
    HiveSemaphores* semaphores; ///< Pointer to the shared semaphore structure for synchronization.
    int semid;     ///< Shared memory identifier for semaphores.
    int shmid;     ///< Shared memory identifier for hive data.
} QueenArgs;

/**
 * queenWorker:
 * The main function executed by the queen process.
 * 
 * Detailed behavior:
 * - Attaches to shared memory for hive data and semaphores.
 * - Periodically lays eggs based on the configured time interval.
 * - Ensures that new bees are added to the hive in a thread-safe manner using semaphores.
 * - Handles insufficient space in the hive by logging warnings.
 * - Cleans up shared memory attachments before termination.
 * 
 * @param arg A pointer to a QueenArgs structure containing the queen's parameters and shared resources.
 */
void queenWorker(QueenArgs* arg);

#endif

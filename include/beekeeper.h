#ifndef BEEKEEPER_H
#define BEEKEEPER_H

#include "common.h"

/**
 * The BeekeeperArgs struct is used to pass necessary data to the beekeeper process.
 * This includes references to shared memory for hive data, semaphores for synchronization,
 * and identifiers required to manage these shared resources.
 */
typedef struct {
    HiveData* hive;            // Pointer to the shared memory structure representing the hive state.
    HiveSemaphores* semaphores; // Pointer to the shared semaphore structure used for synchronization.
    int semid;                 // Shared memory identifier for semaphores.
    int shmid;                 // Shared memory identifier for hive data.
} BeekeeperArgs;

/**
 * beekeeperWorker:
 * This function serves as the main entry point for the beekeeper process.
 * 
 * The beekeeper process monitors and manages the hive's frames, including handling signals
 * to add or remove frames dynamically. The process ensures safe concurrent access
 * using semaphores.
 *
 * Detailed behavior:
 * - Attaches to shared memory segments for hive data and semaphores.
 * - Sets up signal handlers to handle:
 *   1. SIGUSR1: Add frames to the hive.
 *   2. SIGUSR2: Remove frames from the hive.
 *   3. SIGINT: Perform cleanup and release shared memory and semaphores.
 * - Operates in an infinite loop to ensure it remains active and responsive.
 *
 * @param arg A pointer to a BeekeeperArgs structure containing shared memory and synchronization details.
 */
void beekeeperWorker(BeekeeperArgs* arg);

#endif
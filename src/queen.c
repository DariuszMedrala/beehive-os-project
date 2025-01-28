#include "queen.h"
#include "bee.h"
#include "common.h"
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

/**
 * queenWorker:
 * Implements the queen's behavior in the hive simulation.
 * The queen periodically lays eggs and spawns new bees.
 *
 * Includes detailed error handling for semaphore and memory operations.
 *
 * Detailed functionality:
 * 1. Attaches to shared memory for hive data and semaphores.
 * 2. Enters a loop to lay eggs at specified intervals (T_k).
 * 3. Uses semaphores to safely update hive data and spawn bee processes.
 * 4. Checks hive capacity and logs warnings if space is insufficient.
 * 5. Cleans up resources and detaches from shared memory upon termination.
 *
 * @param arg Pointer to QueenArgs containing the queen's configuration and shared resources.
 */
void queenWorker(QueenArgs* arg) {
    QueenArgs* queen = arg;
    prctl(PR_SET_NAME, "queen");

    // Attach to shared memory for hive data and semaphores
    queen->hive = (HiveData*)attachSharedMemory(queen->shmid);
    queen->semaphores = (HiveSemaphores*)attachSharedMemory(queen->semid);
    if (queen->hive == NULL || queen->semaphores == NULL) {
        handleError("[Queen] attachSharedMemory failed", queen->shmid, queen->semid);
    }

    int nextBeeID = queen->hive->N;

    while (1) {
        sleep(queen->T_k); // Wait for the next egg-laying interval

        // Lock hive access
        if (sem_wait(&queen->semaphores->hiveSem) == -1) {
            handleError("[Queen] sem_wait (hiveSem) failed", queen->shmid, queen->semid);
        }
        // Reap any terminated child processes to prevent zombies
        while (waitpid(-1, NULL, WNOHANG) > 0);

        // Calculate available space in the hive
        int freeSpace = calculateP(queen->hive->N) - queen->hive->currentBeesInHive;

        // Check if there is enough space and the total bee count does not exceed hive size N
        if (freeSpace >= queen->eggsCount && (queen->hive->beesAlive + queen->eggsCount) <= queen->hive->N) {
            logMessage(LOG_INFO, "[Queen] Laying %d eggs.", queen->eggsCount);

            for (int i = 0; i < queen->eggsCount; i++) {
                queen->hive->beesAlive++;
                queen->hive->currentBeesInHive++;

                BeeArgs beeArgs = {nextBeeID++, 0, MAX_BEE_VISITS, T_IN_HIVE, queen->hive, queen->semaphores, true, queen->semid, queen->shmid};

                pid_t beePid = fork();
                if (beePid == 0) {
                    beeWorker(&beeArgs); // Start the bee process
                    exit(EXIT_SUCCESS);
                } else if (beePid < 0) {
                    handleError("[Queen] fork failed", queen->shmid, queen->semid);
                }
            }
            logMessage(LOG_INFO, "[Queen] Total living bees: %d", queen->hive->beesAlive);
        } else {
            logMessage(LOG_WARNING, "[Queen] Not enough space in the hive (free: %d) or hive size limit reached (alive: %d, max: %d).", freeSpace, queen->hive->beesAlive, queen->hive->N);
        }

        // Unlock hive access
        if (sem_post(&queen->semaphores->hiveSem) == -1) {
            handleError("[Queen] sem_post (hiveSem) failed", queen->shmid, queen->semid);
        }
    }

    // Detach from shared memory
    detachSharedMemory(queen->hive); // Errors will be logged internally
    detachSharedMemory(queen->semaphores); // Errors will be logged internally
    exit(EXIT_SUCCESS);
}
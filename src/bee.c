#include "bee.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/prctl.h>
#include "common.h"

/**
 * beeWorker:
 * Implements the behavior of a worker bee in the hive simulation.
 *
 * Detailed functionality:
 * 1. Attaches to shared memory for hive data and semaphores.
 * 2. Manages the bee's lifecycle, including entering and exiting the hive.
 * 3. Synchronizes hive access using semaphores to ensure proper concurrent behavior.
 * 4. Logs relevant events such as entering, exiting, and dying.
 * 5. Cleans up shared memory attachments before termination.
 *
 * Includes error handling for semaphore and shared memory operations.
 *
 * @param arg Pointer to BeeArgs containing the bee's individual and shared parameters.
 */
void beeWorker(BeeArgs* arg) {
    BeeArgs* bee = arg;
    char bee_name[16];
    snprintf(bee_name, sizeof(bee_name), "bee_%d", arg->id);
    prctl(PR_SET_NAME, bee_name);

    // Attach to shared memory for hive data and semaphores
    bee->hive = (HiveData*)attachSharedMemory(bee->shmid);
    bee->semaphores = (HiveSemaphores*)attachSharedMemory(bee->semid);
    if (bee->hive == NULL || bee->semaphores == NULL) {
        handleError("[Bee] attachSharedMemory", -1, bee->semid);
    }

    // Initialize random seed for wait time calculations
    unsigned int seed = time(NULL) + getpid();

    // Handle bees born in the hive
    if (bee->startInHive) {
        logMessage(LOG_INFO, "[Bee %d] Starting in the hive.", bee->id);
    
        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        sleep(timeInHive);

        
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Leaving hive initially. (Bees in hive: %d)", bee->id, bee->hive->currentBeesInHive);
        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
         bee->startInHive = false;
    }

    // Main lifecycle of the bee
    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;

        // FIFO logic: Wait in queue for the entrance
        if (sem_wait(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_wait (fifoQueue) failed", -1, bee->semid);
        }

        // Check if the entrance is free
        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            // Release FIFO lock if entrance is not available
            if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
                handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
            }
            continue; // Retry logic will allow trying again
        }

        // Attempt to enter the hive
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
        }
        if (bee->hive->currentBeesInHive >= calculateP(bee->hive->N)) {
            // No space in the hive, release semaphores and wait
            if (sem_post(&bee->semaphores->hiveSem) == -1) {
                handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
            }
            if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
            }
            if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
                handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
            }
            sleep(1); // Wait for a while before retrying
            continue;
        }

        usleep(100000);// Enter the hive
        bee->hive->currentBeesInHive++;
        logMessage(LOG_INFO, "[Bee %d] Entering through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }

        // Exit the FIFO queue
        if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
        }

        // Stay in the hive for a random time
        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        sleep(timeInHive);

        // Leave the hive
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
        }

        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Leaving hive. (Bees in hive: %d)", bee->id, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }

        // Simulate time spent outside the hive
        bee->visits++;
        int sleepTimeOutside = (rand_r(&seed) % (MAX_OUTSIDE_TIME - MIN_OUTSIDE_TIME + 1)) + MIN_OUTSIDE_TIME;
        sleep(sleepTimeOutside); //Wait outside the hive
    }

    // Final steps when the bee "dies"
    if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
    }
    bee->hive->beesAlive--;
    logMessage(LOG_INFO, "[Bee %d] Dying. (Remaining bees: %d)", bee->id, bee->hive->beesAlive);
    if (sem_post(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
    }

    detachSharedMemory(bee->hive);
    detachSharedMemory(bee->semaphores);
    exit(EXIT_SUCCESS);
}

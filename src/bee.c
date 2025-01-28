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
    if (seed == 0) {
        handleError("[Bee] rand_r seed initialization", -1, bee->semid);
    }

    double waitTime = 0.5 + (rand_r(&seed) % 501) / 1000.0;

    if (seed == 0) {
        handleError("[Bee] rand_r seed initialization", -1, bee->semid);
    }

    // If the bee starts in the hive, handle exiting the hive
    while (bee->startInHive) {
        int entrance = rand_r(&seed) % 2;
        logMessage(LOG_INFO, "[Bee %d] Starting life in the hive.", bee->id);

        // Attempt to exit the hive
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
        // If the chosen exit is occupied, try the other one
        entrance = 1 - entrance;
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
        // If both exits are occupied, wait and retry
            sleep(1);
            continue;
        }
        }   
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        usleep(waitTime * 1000000);
        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Exiting through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }

        bee->startInHive = false;
    }

    // Main lifecycle of the bee
    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;

        // Check if the entrance is free
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
            // If the entrance is occupied, choose the other entrance
            entrance = 1 - entrance;
            if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
                // If both entrances are occupied, wait and try again
                sleep(1);
                continue;
            }
        }

        // Attempt to enter the hive
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        // Wait until there is enough space in the hive
        while (bee->hive->currentBeesInHive >= calculateP(bee->hive->N)) {
            if (sem_post(&bee->semaphores->hiveSem) == -1) {
                handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
            }
            if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
            }

            // Wait and retry
            if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Bee] sem_wait (entranceSem)", -1, bee->semid);
            }
            if (sem_wait(&bee->semaphores->hiveSem) == -1) {
                handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
            }
        }

        usleep(waitTime * 1000000);
        bee->hive->currentBeesInHive++;
        logMessage(LOG_INFO, "[Bee %d] Entering through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }

        // Stay in the hive for a random time
        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        unsigned int remainingSleep = sleep(timeInHive);

        if (remainingSleep > 0) {
            handleError("[Bee] sleep interrupted", -1, bee->semid);
        }

        // Attempt to exit the hive
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
        // If the chosen exit is occupied, try the other one
        entrance = 1 - entrance;
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
        // If both exits are occupied, wait and retry
            sleep(1);
            continue;
        }
        }   
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        usleep(waitTime * 1000000);
        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Exiting through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }

        // Increment visit count and wait outside before returning to the hive
        bee->visits++;
        int outsideTime = (rand_r(&seed) % 11) + 10;
        remainingSleep = sleep(outsideTime);
        if (remainingSleep > 0) {
            handleError("[Bee] sleep interrupted", -1, bee->semid);
        }
    }

    // Final steps when the bee "dies"
    if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
    }
    bee->hive->beesAlive--;
    logMessage(LOG_INFO, "[Bee %d] Dying. (Remaining bees: %d)", bee->id, bee->hive->beesAlive);
    if (sem_post(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
    }

    // Detach shared memory and exit
    detachSharedMemory(bee->hive);
    detachSharedMemory(bee->semaphores);
    exit(EXIT_SUCCESS);
}

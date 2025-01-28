#include "bee.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/prctl.h>

/**
 * beeWorker:
 * Implements the behavior of a worker bee in the hive simulation.
 * 
 * Detailed functionality:
 * 1. Attaches to shared memory for hive data and semaphores.
 * 2. Enters and exits the hive through one of two entrances.
 * 3. Sleeps for a random time inside and outside the hive.
 * 4. Logs its activities and updates hive data.
 * 5. Detaches from shared memory and exits upon completing its visits.
 * 
 * @param arg Pointer to BeeArgs containing the bee's configuration and shared resources.
 */
#include "bee.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/prctl.h>

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

    unsigned int seed = time(NULL) + getpid();
    if (seed == 0) {
        handleError("[Bee] rand_r seed initialization", -1, bee->semid);
    }

    double waitTime = MIN_WAIT_TIME + (rand_r(&seed) % (int)((MAX_WAIT_TIME - MIN_WAIT_TIME) * 1000)) / 1000.0;

    if (bee->startInHive) {
        int entrance = rand_r(&seed) % 2;
        logMessage(LOG_INFO, "[Bee %d] Starting life in the hive.", bee->id);

        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

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

    while (bee->visits < MAX_BEE_VISITS) {
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

        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        while (bee->hive->currentBeesInHive >= calculateP(bee->hive->N)) {
            if (sem_post(&bee->semaphores->hiveSem) == -1) {
                handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
            }
            if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
            }

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

        int timeInHive = (rand_r(&seed) % (T_IN_HIVE / 2 + 1)) + T_IN_HIVE;
        unsigned int remainingSleep = sleep(timeInHive);

        if (remainingSleep > 0) {
            handleError("[Bee] sleep interrupted", -1, bee->semid);
        }

        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_wait (entranceSem)", -1, bee->semid);
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

        bee->visits++;
        int outsideTime = (rand_r(&seed) % (MAX_OUTSIDE_TIME - MIN_OUTSIDE_TIME + 1)) + MIN_OUTSIDE_TIME;
        remainingSleep = sleep(outsideTime);
        if (remainingSleep > 0) {
            handleError("[Bee] sleep interrupted", -1, bee->semid);
        }
    }

    if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
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
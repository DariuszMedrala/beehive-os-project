#include "bee.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/prctl.h>
#include "common.h"

/**
 * chooseEntrance:
 * Wybiera wejście na podstawie długości kolejek.
 * Jeśli kolejki są równe, wybiera losowo.
 * Jeśli kolejki są różne, wybiera wejście z krótszą kolejką.
 *
 * @param beesWaiting Tablica z liczbą pszczół czekających na każde wejście.
 * @param seed Ziarno dla generatora liczb losowych.
 * @return Indeks wybranego wejścia (0 lub 1).
 */
int chooseEntrance(int beesWaiting[2], unsigned int* seed) {
    if (abs(beesWaiting[0] - beesWaiting[1]) <= 1) {
        return rand_r(seed) % 2; // Losowo, jeśli kolejki są równe
    } else {
        return (beesWaiting[0] < beesWaiting[1]) ? 0 : 1; // Krótsza kolejka
    }
}

/**
 * beeWorker:
 * Implements the behavior of a worker bee in the hive simulation.
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
    unsigned int seed = (unsigned int)time(NULL) ^ (getpid() << 16) ^ (bee->id << 8);

    // Handle bees born in the hive
    if (bee->startInHive) {
        logMessage(LOG_INFO, "[Bee %d] Starting in the hive.", bee->id);

        // Simulate initial time spent inside the hive
        int timeInHive = (rand_r(&seed) % (1)) + (bee->T_inHive);
        sleep(timeInHive);

        // Lock hive access to update the number of bees in the hive
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }

        // Choose an entrance for exiting, based on the queue length at each entrance
        int entrance = chooseEntrance(bee->hive->beesWaiting, &seed);

        // Increment the count of bees waiting at the selected entrance
        bee->hive->beesWaiting[entrance]++;
        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }

        // Join the FIFO queue at the chosen entrance
        if (sem_wait(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_wait (fifoQueue) failed", -1, bee->semid);
        }

        // Attempt to access the entrance
        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            // Release the FIFO queue semaphore since the entrance is unavailable
            if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
                handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
            }
            // Explicitly handle the case without `continue` since there's no loop
            logMessage(LOG_ERROR, "[Bee %d] Entrance %d unavailable during start, exiting hive aborted.", bee->id, entrance);
            bee->startInHive = false; // Mark as having exited, even if aborted
            return; // Exit the function early for this bee
        }

        // Decrement the count of waiting bees
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }
        bee->hive->beesWaiting[entrance]--;

        // Exit the hive properly through the queue
        usleep(100000);
        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Leaving through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }

        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }

        if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
        }

        bee->startInHive = false; // Mark that the bee has left the hive initially
    }

    // Main lifecycle of the bee
    while (bee->visits < bee->maxVisits) {
        int sleepTimeOutside = (rand_r(&seed) % (MAX_OUTSIDE_TIME - MIN_OUTSIDE_TIME + 1)) + MIN_OUTSIDE_TIME;
        sleep(sleepTimeOutside); 
        // Select an entrance for entering the hive
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
        }

        int entrance = chooseEntrance(bee->hive->beesWaiting, &seed);

        bee->hive->beesWaiting[entrance]++;
        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }

        // Enter the queue for the chosen entrance
        if (sem_wait(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_wait (fifoQueue) failed", -1, bee->semid);
        }

        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
                handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
            }
            continue;
        }

        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }
        bee->hive->beesWaiting[entrance]--;

        // Attempt to enter the hive
        if (bee->hive->currentBeesInHive >= calculateP(bee->hive->N)) {
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

        // Successfully entering the hive
        usleep(100000); // Simulate entry delay
        bee->hive->currentBeesInHive++;
        logMessage(LOG_INFO, "[Bee %d] Entering through entrance %d. (Bees in hive: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->fifoQueue[entrance]) == -1) {
            handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
        }

        // Stay in the hive for a random time
        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        sleep(timeInHive);

        // Exit the hive (same logic as entering)
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
        }

        int leaving = chooseEntrance(bee->hive->beesWaiting, &seed);

        bee->hive->beesWaiting[leaving]++;
        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }

        if (sem_wait(&bee->semaphores->fifoQueue[leaving]) == -1) {
            handleError("[Bee] sem_wait (fifoQueue) failed", -1, bee->semid);
        }

        if (sem_wait(&bee->semaphores->entranceSem[leaving]) == -1) {
            if (sem_post(&bee->semaphores->fifoQueue[leaving]) == -1) {
                handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
            }
            continue;
        }

        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_wait (hiveSem)", -1, bee->semid);
        }
        bee->hive->beesWaiting[leaving]--;

        // Successfully exiting the hive
        usleep(100000);
        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Bee %d] Leaving through entrance %d. (Bees in hive: %d)", bee->id, leaving, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[leaving]) == -1) {
            handleError("[Bee] sem_post (entranceSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->fifoQueue[leaving]) == -1) {
            handleError("[Bee] sem_post (fifoQueue) failed", -1, bee->semid);
        }

        // Simulate time spent outside the hive
        bee->visits++;
    }

    // Final steps when the bee "dies"
    if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_wait (hiveSem) failed", -1, bee->semid);
    }

    // Decrease the number of alive bees
    bee->hive->beesAlive--;
    logMessage(LOG_INFO, "[Bee %d] Dying. (Remaining bees: %d)", bee->id, bee->hive->beesAlive);

    if (sem_post(&bee->semaphores->hiveSem) == -1) {
        handleError("[Bee] sem_post (hiveSem)", -1, bee->semid);
    }

    // Detach from shared memory
    detachSharedMemory(bee->hive);
    detachSharedMemory(bee->semaphores);

    // Exit the bee process
    exit(EXIT_SUCCESS);
}
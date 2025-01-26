#include "bee.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>

void beeWorker(BeeArgs* arg) {
    BeeArgs* bee = arg;
    HiveData* hive = bee->hive;

    // Użyj funkcji pomocniczej do dołączenia pamięci współdzielonej dla semaforów
    HiveSemaphores* semaphores = (HiveSemaphores*)attachSharedMemory(bee->semid);
    if (semaphores == NULL) {
        handleError("[Pszczoła] attachSharedMemory", -1, bee->semid);
    }

    unsigned int seed = time(NULL) + bee->id;
    if (seed == 0) {
        handleError("[Pszczoła] rand_r seed initialization", -1, bee->semid);
    }

    if (bee->startInHive) {
        int entrance = rand_r(&seed) % 2;
        printf("[Pszczoła %d] Rozpoczyna życie w ulu.\n", bee->id);

        if (sem_wait(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        if (sem_post(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        bee->startInHive = false;
    }

    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;

        if (sem_wait(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        while (hive->currentBeesInHive >= hive->P) {
            if (sem_post(&semaphores->hiveSem) == -1) {
                handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
            }
            if (sem_post(&semaphores->entranceSem[entrance]) == -1) {
                handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
            }
            sleep(1);
            if (sem_wait(&semaphores->entranceSem[entrance]) == -1) {
                handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
            }
            if (sem_wait(&semaphores->hiveSem) == -1) {
                handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
            }
        }

        hive->currentBeesInHive++;
        printf("[Pszczoła %d] Wchodzi przez wejście/wyjście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        if (sem_post(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        unsigned int remainingSleep = sleep(timeInHive);
        if (remainingSleep > 0) {
            handleError("[Pszczoła] sleep interrupted", -1, bee->semid);
        }

        if (sem_wait(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        if (sem_post(&semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        bee->visits++;
        int outsideTime = (rand_r(&seed) % 11) + 10;
        remainingSleep = sleep(outsideTime);
        if (remainingSleep > 0) {
            handleError("[Pszczoła] sleep interrupted", -1, bee->semid);
        }
    }

    if (sem_wait(&semaphores->hiveSem) == -1) {
        handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
    }
    hive->beesAlive--;
    printf("[Pszczoła %d] Umiera. (Pozostało pszczół: %d)\n", bee->id, hive->beesAlive);
    if (sem_post(&semaphores->hiveSem) == -1) {
        handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
    }

    // Odłącz pamięć współdzieloną
    detachSharedMemory(semaphores);
    exit(EXIT_SUCCESS);
}

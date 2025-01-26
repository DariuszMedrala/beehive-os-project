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

    // Dołącz pamięć współdzieloną tylko raz
    bee->hive = (HiveData*)attachSharedMemory(bee->shmid);
    bee->semaphores = (HiveSemaphores*)attachSharedMemory(bee->semid);
    if (bee->hive == NULL || bee->semaphores == NULL) {
        handleError("[Pszczoła] attachSharedMemory", -1, bee->semid);
    }

    unsigned int ziarno = time(NULL) + getpid();
    if (ziarno == 0) {
        handleError("[Pszczoła] rand_r seed initialization", -1, bee->semid);
    }

    double waitTime = 0.5 + (rand_r(&ziarno) % 501) / 1000.0;

    unsigned int seed = time(NULL) + bee->id;
    if (seed == 0) {
        handleError("[Pszczoła] rand_r seed initialization", -1, bee->semid);
    }

    if (bee->startInHive) {
        int entrance = rand_r(&seed) % 2;
        logMessage(LOG_INFO, "[Pszczoła %d] Rozpoczyna życie w ulu.", bee->id);

        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        bee->startInHive = false;
    }

    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;

        // Sprawdź, czy wyjście jest wolne
        if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
            // Jeśli wyjście jest zajęte, wybierz drugie wyjście
            entrance = 1 - entrance;
            if (sem_trywait(&bee->semaphores->entranceSem[entrance]) == -1) {
                // Jeśli oba wyjścia są zajęte, poczekaj chwilę i spróbuj ponownie
                sleep(1);
                continue;
            }
        }

        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        while (bee->hive->currentBeesInHive >= calculateP(bee->hive->N)) {
            if (sem_post(&bee->semaphores->hiveSem) == -1) {
                handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
            }
            if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
            }

            if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
                handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
            }
            if (sem_wait(&bee->semaphores->hiveSem) == -1) {
                handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
            }
        }

        usleep(waitTime * 1000000);
        bee->hive->currentBeesInHive++;
        logMessage(LOG_INFO, "[Pszczoła %d] Wchodzi przez wejście/wyjście %d. (W ulu: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        unsigned int remainingSleep = sleep(timeInHive);

        if (remainingSleep > 0) {
            handleError("[Pszczoła] sleep interrupted", -1, bee->semid);
        }

        if (sem_wait(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_wait (entranceSem)", -1, bee->semid);
        }
        if (sem_wait(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
        }

        usleep(waitTime * 1000000);
        bee->hive->currentBeesInHive--;
        logMessage(LOG_INFO, "[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)", bee->id, entrance, bee->hive->currentBeesInHive);

        if (sem_post(&bee->semaphores->hiveSem) == -1) {
            handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
        }
        if (sem_post(&bee->semaphores->entranceSem[entrance]) == -1) {
            handleError("[Pszczoła] sem_post (entranceSem)", -1, bee->semid);
        }

        bee->visits++;
        int outsideTime = (rand_r(&seed) % 11) + 10;
        remainingSleep = sleep(outsideTime);
        if (remainingSleep > 0) {
            handleError("[Pszczoła] sleep interrupted", -1, bee->semid);
        }
    }

    if (sem_wait(&bee->semaphores->hiveSem) == -1) {
        handleError("[Pszczoła] sem_wait (hiveSem)", -1, bee->semid);
    }
    bee->hive->beesAlive--;
    logMessage(LOG_INFO, "[Pszczoła %d] Umiera. (Pozostało pszczół: %d)", bee->id, bee->hive->beesAlive);
    if (sem_post(&bee->semaphores->hiveSem) == -1) {
        handleError("[Pszczoła] sem_post (hiveSem)", -1, bee->semid);
    }

    detachSharedMemory(bee->hive);
    detachSharedMemory(bee->semaphores);
    exit(EXIT_SUCCESS);
}
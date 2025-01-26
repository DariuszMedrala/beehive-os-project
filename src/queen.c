#include "queen.h"
#include "bee.h"
#include "common.h"
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

void queenWorker(QueenArgs* arg) {
    QueenArgs* queen = arg;
    prctl(PR_SET_NAME, "queen");

    // Dołącz pamięć współdzieloną tylko raz
    queen->hive = (HiveData*)attachSharedMemory(queen->shmid);
    queen->semaphores = (HiveSemaphores*)attachSharedMemory(queen->semid);
    if (queen->hive == NULL || queen->semaphores == NULL) {
        handleError("[Królowa] attachSharedMemory", -1, queen->semid);
    }

    int nextBeeID = queen->hive->N;

    while (1) {
        sleep(queen->T_k);

        if (sem_wait(&queen->semaphores->hiveSem) == -1) {
            handleError("[Królowa] sem_wait (hiveSem)", -1, queen->semid);
        }

        int wolneMiejsce = calculateP(queen->hive->N) - queen->hive->currentBeesInHive;
        if (wolneMiejsce >= queen->eggsCount && queen->eggsCount < (queen->hive->N - queen->hive->beesAlive)) {
            logMessage(LOG_INFO, "[Królowa] Składa %d jaja.", queen->eggsCount);

            for (int i = 0; i < queen->eggsCount; i++) {
                queen->hive->beesAlive++;
                queen->hive->currentBeesInHive++;

                BeeArgs beeArgs = {nextBeeID++, 0, 3, 10, queen->hive, queen->semaphores, false, queen->semid, queen->shmid};

                pid_t beePid = fork();
                if (beePid == 0) {
                    beeWorker(&beeArgs);
                    exit(EXIT_SUCCESS);
                } else if (beePid < 0) {
                    handleError("[Królowa] fork", -1, queen->semid);
                }
            }
            logMessage(LOG_INFO, "[Królowa] Teraz żywych pszczół: %d", queen->hive->beesAlive);
        } else {
            logMessage(LOG_WARNING, "[Królowa] Za mało miejsca w ulu (wolne: %d) lub brak miejsca w kolonii.", wolneMiejsce);
        }

        if (sem_post(&queen->semaphores->hiveSem) == -1) {
            handleError("[Królowa] sem_post (hiveSem)", -1, queen->semid);
        }
    }

    detachSharedMemory(queen->hive);
    detachSharedMemory(queen->semaphores);
    exit(EXIT_SUCCESS);
}
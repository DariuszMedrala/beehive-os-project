#include "queen.h"
#include "bee.h"
#include "common.h"
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void queenWorker(QueenArgs* arg) {
    QueenArgs* queen = arg;
    HiveData* hive = queen->hive;

    // Użyj funkcji pomocniczej do dołączenia pamięci współdzielonej dla semaforów
    HiveSemaphores* semaphores = (HiveSemaphores*)attachSharedMemory(queen->semid);
    if (semaphores == NULL) {
        exit(EXIT_FAILURE);
    }

    int nextBeeID = hive->N;

    while (1) {
        sleep(queen->T_k);

        sem_wait(&semaphores->hiveSem);

        int wolneMiejsce = hive->P - hive->currentBeesInHive;
        if (wolneMiejsce >= queen->eggsCount && queen->eggsCount < (hive->N - hive->beesAlive)) {
            coloredPrintf(GREEN, "[Królowa] Składa %d jaja.\n", queen->eggsCount);

            for (int i = 0; i < queen->eggsCount; i++) {
                hive->beesAlive++;
                hive->currentBeesInHive++;

                BeeArgs beeArgs = {nextBeeID++, 0, 3, 60, hive, false, queen->semid}; // Przekaż semid do pszczoły

                pid_t beePid = fork();
                if (beePid == 0) {
                    beeWorker(&beeArgs);
                    exit(EXIT_SUCCESS);
                } else if (beePid < 0) {
                    perror("[Królowa] fork");
                    hive->beesAlive--;
                }
            }
            printf("[Królowa] Teraz żywych pszczół: %d\n", hive->beesAlive);
        } else {
            printf("[Królowa] Za mało miejsca w ulu (wolne: %d) lub brak miejsca w kolonii.\n", wolneMiejsce);
        }

        sem_post(&semaphores->hiveSem);
    }

    // Odłącz pamięć współdzieloną
    detachSharedMemory(semaphores);
    exit(EXIT_SUCCESS);
}
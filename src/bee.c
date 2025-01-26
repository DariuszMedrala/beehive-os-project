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
        exit(EXIT_FAILURE);
    }

    unsigned int seed = time(NULL) + bee->id;

    if (bee->startInHive) {
        int entrance = rand_r(&seed) % 2;
        printf("[Pszczoła %d] Rozpoczyna życie w ulu.\n", bee->id);

        // Zajmij semafor wejścia/wyjścia
        sem_wait(&semaphores->entranceSem[entrance]);
        sem_wait(&semaphores->hiveSem);

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        sem_post(&semaphores->hiveSem);
        // Zwolnij semafor wejścia/wyjścia
        sem_post(&semaphores->entranceSem[entrance]);

        bee->startInHive = false;
    }

    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;

        // Zajmij semafor wejścia/wyjścia
        sem_wait(&semaphores->entranceSem[entrance]);
        // Sprawdź, czy w ulu jest miejsce
        sem_wait(&semaphores->hiveSem);
        while (hive->currentBeesInHive >= hive->P) {
            sem_post(&semaphores->hiveSem); // Zwolnij semafor hiveSem, aby inne procesy mogły działać
            sem_post(&semaphores->entranceSem[entrance]); // Zwolnij semafor wejścia
            sleep(1); // Poczekaj chwilę
            sem_wait(&semaphores->entranceSem[entrance]); // Ponownie zajmij semafor wejścia
            sem_wait(&semaphores->hiveSem); // Ponownie zajmij semafor hiveSem
        }

        hive->currentBeesInHive++;
        printf("[Pszczoła %d] Wchodzi przez wejście/wyjście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        sem_post(&semaphores->hiveSem);
        // Zwolnij semafor wejścia/wyjścia
        sem_post(&semaphores->entranceSem[entrance]);

        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive);
        sleep(timeInHive);

        // Zajmij semafor wejścia/wyjścia
        sem_wait(&semaphores->entranceSem[entrance]);
        sem_wait(&semaphores->hiveSem);

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n", bee->id, entrance, hive->currentBeesInHive);

        sem_post(&semaphores->hiveSem);
        // Zwolnij semafor wejścia/wyjścia
        sem_post(&semaphores->entranceSem[entrance]);

        bee->visits++;
        int outsideTime = (rand_r(&seed) % 11) + 10;
        sleep(outsideTime);
    }

    sem_wait(&semaphores->hiveSem);
    hive->beesAlive--;
    printf("[Pszczoła %d] Umiera. (Pozostało pszczół: %d)\n", bee->id, hive->beesAlive);
    sem_post(&semaphores->hiveSem);

    // Odłącz pamięć współdzieloną
    detachSharedMemory(semaphores);
    exit(EXIT_SUCCESS);
}
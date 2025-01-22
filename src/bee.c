#include "bee.h"

void* beeWorker(void* arg) {
    BeeArgs* bee = (BeeArgs*)arg;
    HiveData* hive = bee->hive;

    // Różne ziarno losowe dla każdej pszczoły (oparte np. o id)
    srand(time(NULL) + bee->id);

    while (bee->visits < bee->maxVisits) {
        int entrance = rand() % 2;  // Wybór jednego z dwóch wejść

        // Próba wejścia do ula
        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_lock");
            pthread_exit(NULL);
        }

        while (hive->entranceInUse[entrance] == true ||
               hive->currentBeesInHive >= hive->maxCapacity)
        {
            if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
                perror("[Pszczoła] pthread_mutex_unlock (waiting)");
                pthread_exit(NULL);
            }
            usleep(100000); // 0.1 sek przerwy
            if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
                perror("[Pszczoła] pthread_mutex_lock (waiting)");
                pthread_exit(NULL);
            }
        }

        // Zajmujemy wejście
        hive->entranceInUse[entrance] = true;
        hive->currentBeesInHive++;
        printf("[Pszczoła %d] Wchodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (enter)");
            pthread_exit(NULL);
        }

        // Przebywanie w ulu
        sleep(bee->T_inHive);

        // Wyjście z ula
        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (exit)");
            pthread_exit(NULL);
        }

        hive->currentBeesInHive--;
        hive->entranceInUse[entrance] = false;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (exit)");
            pthread_exit(NULL);
        }

        bee->visits++;
        // Czas spędzony poza ulem
        int outsideTime = (rand() % 3) + 1; // 1-3 sek
        sleep(outsideTime);
    }

    // Pszczoła umiera
    if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
        perror("[Pszczoła] pthread_mutex_lock (die)");
        pthread_exit(NULL);
    }
    hive->beesAlive--;
    printf("[Pszczoła %d] Umiera. (Pozostało pszczół: %d)\n", bee->id, hive->beesAlive);
    if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
        perror("[Pszczoła] pthread_mutex_unlock (die)");
        pthread_exit(NULL);
    }

    free(bee); // zwalniamy pamięć BeeArgs
    pthread_exit(NULL);
}

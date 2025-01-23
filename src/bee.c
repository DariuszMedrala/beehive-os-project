#include "bee.h"

void* beeWorker(void* arg) {
    BeeArgs* bee = (BeeArgs*)arg;
    HiveData* hive = bee->hive;
    

    // Różne ziarno losowe dla każdej pszczoły (oparte np. o id)
    unsigned int seed = time(NULL) + bee->id;

    // Pszczoła rozpoczyna od przebywania na zewnątrz
    

    if (bee->startInHive) {
         int entrance = rand_r(&seed) % 2;  // Wybór jednego z dwóch wejść
        // Pszczoła zaczyna życie w ulu i najpierw próbuje wyjść
        printf("[Pszczoła %d] Rozpoczynam życie w ulu.\n", bee->id);
        sleep((rand_r(&seed) % 5) + 1);

                // Wyjście z ula
        if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (exit entrance)");
            pthread_exit(NULL);
        }

        while (hive->entranceInUse[entrance] == true) {
            if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_unlock (waiting exit)");
                pthread_exit(NULL);
            }

            entrance = 1 - entrance; // Przełącz wyjście

            if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_lock (switch exit)");
                pthread_exit(NULL);
            }
        }

        // Zajmujemy wejście przed wyjściem
        printf("[Pszczoła %d] Zajmuje wejście/wyjście: %d przed wyjściem.\n",
               bee->id, entrance);
        hive->entranceInUse[entrance] = true;

        // Symulacja czasu wyjścia (1 sekunda)
        sleep(2);

        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (hive exit)");
            pthread_exit(NULL);
        }

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (hive exit)");
            pthread_exit(NULL);
        }

        // Zwolnienie wejścia po wyjściu
        hive->entranceInUse[entrance] = false;
        printf("[Pszczoła %d] Zwalnia wejście/wyjście: %d po wyjściu.\n",
               bee->id, entrance);

        if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (exit)");
            pthread_exit(NULL);
        }
        bee->startInHive = false;
    } else {
        sleep((rand_r(&seed) % 5) + 1); // Symulacja czasu poza ulem
    }

    while (bee->visits < bee->maxVisits) {
        int entrance = rand_r(&seed) % 2;  // Wybór jednego z dwóch wejść

        // Próba wejścia do ula
        if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (entrance)");
            pthread_exit(NULL);
        }

        while (hive->entranceInUse[entrance] == true || 
               hive->currentBeesInHive >= (hive->P - 1)) {
            if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_unlock (waiting entrance)");
                pthread_exit(NULL);
            }

            entrance = 1 - entrance; // Przełącz wejście

            if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_lock (switch entrance)");
                pthread_exit(NULL);
            }
        }

        // Zajmujemy wejście
        printf("[Pszczoła %d] Zajmuje wejście/wyjście: %d.\n",
               bee->id, entrance);
        hive->entranceInUse[entrance] = true;

        // Symulacja czasu wejścia (1 sekunda)
        sleep(2);

        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (hive)");
            pthread_exit(NULL);
        }

        hive->currentBeesInHive++;
        printf("[Pszczoła %d] Wchodzi przez wejście/wyjście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (hive)");
            pthread_exit(NULL);
        }

        // Zwolnienie wejścia natychmiast po wejściu
        hive->entranceInUse[entrance] = false;
        printf("[Pszczoła %d] Zwalnia wejście/wyjście: %d.\n",
               bee->id, entrance);

        if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (enter)");
            pthread_exit(NULL);
        }

        // Przebywanie w ulu
        int timeInHive = (rand_r(&seed) % (bee->T_inHive / 2 + 1)) + (bee->T_inHive); // T_inHive/2 do T_inHive sekund
        sleep(timeInHive);

        // Wyjście z ula
        if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (exit entrance)");
            pthread_exit(NULL);
        }

        while (hive->entranceInUse[entrance] == true) {
            if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_unlock (waiting exit)");
                pthread_exit(NULL);
            }

            entrance = 1 - entrance; // Przełącz wyjście

            if (pthread_mutex_lock(&hive->entranceMutex[entrance]) != 0) {
                perror("[Pszczoła] pthread_mutex_lock (switch exit)");
                pthread_exit(NULL);
            }
        }

        // Zajmujemy wejście przed wyjściem
        printf("[Pszczoła %d] Zajmuje wejście/wyjście: %d przed wyjściem.\n",
               bee->id, entrance);
        hive->entranceInUse[entrance] = true;

        // Symulacja czasu wyjścia (1 sekunda)
        sleep(2);

        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_lock (hive exit)");
            pthread_exit(NULL);
        }

        hive->currentBeesInHive--;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (hive exit)");
            pthread_exit(NULL);
        }

        // Zwolnienie wejścia po wyjściu
        hive->entranceInUse[entrance] = false;
        printf("[Pszczoła %d] Zwalnia wejście/wyjście: %d po wyjściu.\n",
               bee->id, entrance);

        if (pthread_mutex_unlock(&hive->entranceMutex[entrance]) != 0) {
            perror("[Pszczoła] pthread_mutex_unlock (exit)");
            pthread_exit(NULL);
        }

        bee->visits++;
        // Czas spędzony poza ulem
        int outsideTime = (rand_r(&seed) % 11) + 10; // 10-20 sek
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

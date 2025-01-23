#include "queen.h"
#include "bee.h" // żeby móc tworzyć nowe wątki pszczół

void* queenWorker(void* arg) {
    QueenArgs* queen = (QueenArgs*)arg;
    HiveData* hive = queen->hive;

    // Zmienna statyczna lub globalna do generowania unikalnych ID nowo narodzonych pszczół
    static int nextBeeID = 1000; 

    while (1) {
        sleep(queen->T_k); // co T_k sekund

        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Królowa] pthread_mutex_lock");
            break; // lub pthread_exit(NULL)
        }

        int wolneMiejsce = hive->maxCapacity - hive->currentBeesInHive;
        if (wolneMiejsce >= queen->eggsCount) {
            printf("[Królowa] Składa %d jaj.\n", queen->eggsCount);
            // Tworzymy faktyczne wątki pszczół
            for (int i = 0; i < queen->eggsCount; i++) {
                // Zwiększamy beesAlive
                hive->beesAlive++;

                // Tworzymy nową strukturę BeeArgs dla każdej nowej pszczoły
                BeeArgs* newBee = (BeeArgs*)malloc(sizeof(BeeArgs));
                if (!newBee) {
                    perror("[Królowa] malloc newBee");
                    hive->beesAlive--;
                    continue;
                }
                newBee->id = nextBeeID++;
                newBee->visits = 0;
                newBee->maxVisits = 3;   // nowo narodzona pszczoła też żyje do 3 wizyt
                newBee->T_inHive = 2;    // i spędza 2 sek w ulu
                newBee->hive = hive;

                // Tworzymy wątek nowej pszczoły
                pthread_t newBeeThread;
                if (pthread_create(&newBeeThread, NULL, beeWorker, newBee) != 0) {
                    perror("[Królowa] pthread_create newBee");
                    hive->beesAlive--;
                    free(newBee);
                } else {
                    // żeby nie musieć wywoływać pthread_join() dla nowej pszczoły
                    pthread_detach(newBeeThread);
                }
            }
            printf("[Królowa] Teraz żywych pszczół: %d\n", hive->beesAlive);
        } else {
            printf("[Królowa] Za mało miejsca w ulu (wolne: %d).\n", wolneMiejsce);
        }

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Królowa] pthread_mutex_unlock");
            break;
        }
    }

    pthread_exit(NULL);
}

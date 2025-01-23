#include "common.h"
#include "bee.h"
#include "queen.h"
#include "beekeeper.h"

void initHiveData(HiveData* hive, int N, int P);

int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Użycie: %s <N> <P> <T_k> <eggsCount> <liczba_pszczol_robotnic>\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int P = atoi(argv[2]);
    int T_k = atoi(argv[3]);
    int eggsCount = atoi(argv[4]);
    int workerBeesCount = atoi(argv[5]);

    if (N <= 0 || P <= 0 || P >= N/2 || T_k <= 0 || eggsCount <= 0 || workerBeesCount <= 0) {
        fprintf(stderr, "Błędne wartości. Upewnij się, że P < N/2 i wszystko > 0.\n");
        return 1;
    }

    // Przykład otwarcia pliku logów z obsługą błędów
    int logFile = open("beehive.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFile < 0) {
        perror("[MAIN] open(\"beehive.log\")");
        return 1;
    }
    const char* startMsg = "Beehive simulation started.\n";
    if (write(logFile, startMsg, strlen(startMsg)) < 0) {
        perror("[MAIN] write(beehive.log)");
    }
    if (close(logFile) < 0) {
        perror("[MAIN] close(beehive.log)");
    }

    // Inicjalizacja danych ula
    HiveData hive;
    initHiveData(&hive, N, P);

    // Tworzenie wątku królowej
    pthread_t queenThread;
    QueenArgs* queenArgs = malloc(sizeof(QueenArgs));
    if (!queenArgs) {
        perror("[MAIN] malloc queenArgs");
        return 1;
    }
    queenArgs->T_k = T_k;
    queenArgs->eggsCount = eggsCount;
    queenArgs->hive = &hive;

    if (pthread_create(&queenThread, NULL, queenWorker, queenArgs) != 0) {
        perror("[MAIN] pthread_create queen");
        free(queenArgs);
        return 1;
    }

    // Tworzenie wątku pszczelarza
    pthread_t beekeeperThread;
    BeekeeperArgs* keeperArgs = malloc(sizeof(BeekeeperArgs));
    if (!keeperArgs) {
        perror("[MAIN] malloc keeperArgs");
        free(queenArgs);
        return 1;
    }
    keeperArgs->hive = &hive;
    if (pthread_create(&beekeeperThread, NULL, beekeeperWorker, keeperArgs) != 0) {
        perror("[MAIN] pthread_create beekeeper");
        free(queenArgs);
        free(keeperArgs);
        return 1;
    }

    // Tworzenie wątków pszczół robotnic (początkowych)
    pthread_t* beeThreads = malloc(sizeof(pthread_t) * workerBeesCount);
    if (!beeThreads) {
        perror("[MAIN] malloc beeThreads");
        free(queenArgs);
        free(keeperArgs);
        return 1;
    }

    for (int i = 0; i < workerBeesCount; i++) {
        BeeArgs* b = malloc(sizeof(BeeArgs));
        if (!b) {
            perror("[MAIN] malloc BeeArgs");
            // Możemy pominąć lub obsłużyć awaryjnie
            continue;
        }
        b->id = i;      // ID od 0 do workerBeesCount-1
        b->visits = 0;
        b->maxVisits = 3;  // po ilu wizytach pszczoła umiera
        b->T_inHive = 2;   // czas w ulu
        b->hive = &hive;

        // Zwiększamy beesAlive
        if (pthread_mutex_lock(&hive.hiveMutex) != 0) {
            perror("[MAIN] pthread_mutex_lock (beesAlive++)");
            free(b);
            continue;
        }
        hive.beesAlive++;
        if (pthread_mutex_unlock(&hive.hiveMutex) != 0) {
            perror("[MAIN] pthread_mutex_unlock (beesAlive++)");
            free(b);
            continue;
        }

        if (pthread_create(&beeThreads[i], NULL, beeWorker, b) != 0) {
            perror("[MAIN] pthread_create beeWorker");
            free(b);
        }
    }

    // Czekamy, aż początkowe wątki pszczół skończą życie
    for (int i = 0; i < workerBeesCount; i++) {
        pthread_join(beeThreads[i], NULL);
    }
    free(beeThreads);

    // Jednak królowa i pszczelarz działają w nieskończoność:
    // => czekamy na ich zakończenie (co nie nastąpi, póki nie przerwiemy programu).
    pthread_join(queenThread, NULL);
    pthread_join(beekeeperThread, NULL);

    free(queenArgs);
    free(keeperArgs);

    if (pthread_mutex_destroy(&hive.hiveMutex) != 0) {
        perror("[MAIN] pthread_mutex_destroy");
    }

    printf("[MAIN] Koniec symulacji. (Praktycznie nigdy tu nie dojdzie)\n");
    return 0;
}

void initHiveData(HiveData* hive, int N, int P) {
    hive->currentBeesInHive = 0;
    hive->maxCapacity = P;
    hive->N = N;
    hive->P = P;
    hive->entranceInUse[0] = false;
    hive->entranceInUse[1] = false;
    hive->beesAlive = 0;

    if (pthread_mutex_init(&hive->hiveMutex, NULL) != 0) {
        perror("[MAIN] pthread_mutex_init(hiveMutex)");
        exit(EXIT_FAILURE);
    }
}

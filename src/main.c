#include "common.h"
#include "bee.h"
#include "queen.h"
#include "beekeeper.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

void initHiveData(HiveData* hive, int N, int P);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Poddaj argumenty: %s <N: początkowa liczba pszczół w roju.> <T_k: czas co jaki królowa składa jaja.> <eggsCount: jaką ilośc jaj ma złożyć.> \n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int T_k = atoi(argv[2]);
    int eggsCount = atoi(argv[3]);

    int P = (N / 2) - 1;

    if (N <= 0 || T_k <= 0 || eggsCount <= 0) {
        fprintf(stderr, "Błędne wartości. Upewnij się, że wszystko > 0.\n");
        return 1;
    }

    // Tworzenie pamięci współdzielonej dla danych ula
    int shmid = shmget(IPC_PRIVATE, sizeof(HiveData), IPC_CREAT | 0666);
    if (shmid == -1) {
        handleError("[MAIN] shmget", -1, -1);
    }

    HiveData* hive = (HiveData*)attachSharedMemory(shmid);
    if (hive == NULL) {
        handleError("[MAIN] shmat (HiveData)", shmid, -1);
    }

    initHiveData(hive, N, P);

    // Tworzenie pamięci współdzielonej dla semaforów
    int semid = shmget(IPC_PRIVATE, sizeof(HiveSemaphores), IPC_CREAT | 0666);
    if (semid == -1) {
        handleError("[MAIN] shmget (semaphores)", shmid, -1);
    }

    HiveSemaphores* semaphores = (HiveSemaphores*)attachSharedMemory(semid);
    if (semaphores == NULL) {
        handleError("[MAIN] shmat (semaphores)", shmid, semid);
    }

    if (sem_init(&semaphores->hiveSem, 1, 1) == -1) {
        handleError("[MAIN] sem_init (hiveSem)", shmid, semid);
    }

    for (int i = 0; i < 2; i++) {
        if (sem_init(&semaphores->entranceSem[i], 1, 1) == -1) {
            handleError("[MAIN] sem_init (entranceSem)", shmid, semid);
        }
    }

    // Uruchomienie procesu królowej
    pid_t queenPid = fork();
    if (queenPid == 0) {
        QueenArgs queenArgs = {T_k, eggsCount, NULL, NULL, semid, shmid};
        queenWorker(&queenArgs);
        exit(EXIT_SUCCESS);
    } else if (queenPid < 0) {
        handleError("[MAIN] fork (queen)", shmid, semid);
    }

    // Uruchomienie procesu pszczelarza
    pid_t beekeeperPid = fork();
    if (beekeeperPid == 0) {
        BeekeeperArgs keeperArgs = {NULL, NULL, semid, shmid};
        beekeeperWorker(&keeperArgs);
        exit(EXIT_SUCCESS);
    } else if (beekeeperPid < 0) {
        handleError("[MAIN] fork (beekeeper)", shmid, semid);
    }

    // Uruchomienie procesów pszczół
    for (int i = 0; i < N; i++) {
        pid_t beePid = fork();
        if (beePid == 0) {
            BeeArgs beeArgs = {i, 0, 3, 10, NULL, NULL, false, semid, shmid};
            beeWorker(&beeArgs);
            exit(EXIT_SUCCESS);
        } else if (beePid < 0) {
            handleError("[MAIN] fork (bee)", shmid, semid);
        }
    }

    // Czekanie na zakończenie procesów pszczół
    for (int i = 0; i < N; i++) {
        if (wait(NULL) == -1) {
            handleError("[MAIN] wait", shmid, semid);
        }
    }

    // Zakończenie procesów królowej i pszczelarza
    if (kill(queenPid, SIGTERM) == -1) {
        handleError("[MAIN] kill (queen)", shmid, semid);
    }
    if (kill(beekeeperPid, SIGTERM) == -1) {
        handleError("[MAIN] kill (beekeeper)", shmid, semid);
    }

    // Usuwanie pamięci współdzielonej i semaforów
    detachSharedMemory(hive);
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        handleError("[MAIN] shmctl IPC_RMID (HiveData)", shmid, semid);
    }

    detachSharedMemory(semaphores);
    if (shmctl(semid, IPC_RMID, NULL) == -1) {
        handleError("[MAIN] shmctl IPC_RMID (semaforów)", shmid, semid);
    }

    logMessage(LOG_INFO, "[MAIN] Koniec symulacji.");
    return 0;
}

void initHiveData(HiveData* hive, int N, int P) {
    hive->currentBeesInHive = 0;
    hive->N = N;
    hive->P = P;
    hive->beesAlive = 0;
}
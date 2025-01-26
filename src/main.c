#include "common.h"
#include "bee.h"
#include "queen.h"
#include "beekeeper.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

void initHiveData(HiveData* hive, int N, int P);
void initSemaphores(HiveSemaphores* semaphores);

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
        perror("[MAIN] shmget");
        return 1;
    }

    HiveData* hive = (HiveData*)shmat(shmid, NULL, 0);
    if (hive == (void*)-1) {
        perror("[MAIN] shmat");
        return 1;
    }

    initHiveData(hive, N, P);

    // Tworzenie pamięci współdzielonej dla semaforów
    int semid = shmget(IPC_PRIVATE, sizeof(HiveSemaphores), IPC_CREAT | 0666);
    if (semid == -1) {
        perror("[MAIN] shmget (semaphores)");
        return 1;
    }

    HiveSemaphores* semaphores = (HiveSemaphores*)shmat(semid, NULL, 0);
    if (semaphores == (void*)-1) {
        perror("[MAIN] shmat (semaphores)");
        return 1;
    }

    initSemaphores(semaphores);

    // Uruchomienie procesu królowej
    pid_t queenPid = fork();
    if (queenPid == 0) {
        QueenArgs queenArgs = {T_k, eggsCount, hive, semid}; // Przekaż semid
        queenWorker(&queenArgs);
        exit(EXIT_SUCCESS);
    } else if (queenPid < 0) {
        perror("[MAIN] fork (queen)");
        return 1;
    }

    // Uruchomienie procesu pszczelarza
    pid_t beekeeperPid = fork();
    if (beekeeperPid == 0) {
        BeekeeperArgs keeperArgs = {hive, semid}; // Przekaż semid
        beekeeperWorker(&keeperArgs);
        exit(EXIT_SUCCESS);
    } else if (beekeeperPid < 0) {
        perror("[MAIN] fork (beekeeper)");
        return 1;
    }

    // Uruchomienie procesów pszczół
    for (int i = 0; i < N; i++) {
        pid_t beePid = fork();
        if (beePid == 0) {
            BeeArgs beeArgs = {i, 0, 3, 60, hive, false, semid}; // Przekaż semid
            beeWorker(&beeArgs);
            exit(EXIT_SUCCESS);
        } else if (beePid < 0) {
            perror("[MAIN] fork (bee)");
        }
    }

    // Czekanie na zakończenie procesów pszczół
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    // Zakończenie procesów królowej i pszczelarza
    kill(queenPid, SIGTERM);
    kill(beekeeperPid, SIGTERM);

    // Usuwanie pamięci współdzielonej i semaforów
    shmdt(hive);
    shmctl(shmid, IPC_RMID, NULL);

    shmdt(semaphores);
    shmctl(semid, IPC_RMID, NULL);

    printf("[MAIN] Koniec symulacji.\n");
    return 0;
}

void initHiveData(HiveData* hive, int N, int P) {
    hive->currentBeesInHive = 0;
    hive->N = N;
    hive->P = P;
    hive->beesAlive = 0;
}

void initSemaphores(HiveSemaphores* semaphores) {
    if (sem_init(&semaphores->hiveSem, 1, 1) == -1) {
        perror("[MAIN] sem_init (hiveSem)");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 2; i++) {
        if (sem_init(&semaphores->entranceSem[i], 1, 1) == -1) {
            perror("[MAIN] sem_init (entranceSem)");
            exit(EXIT_FAILURE);
        }
    }
}
#include "beekeeper.h"
#include <string.h>
#include <signal.h>
#include <common.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

// Funkcja pomocnicza do uzyskania wskaźnika na HiveData i semafory
HiveData* getHiveDataAndSemaphores(HiveSemaphores** semaphores) {
    if (gBeekeeperArgs == NULL) {
        return NULL;
    }

    *semaphores = (HiveSemaphores*)shmat(gBeekeeperArgs->semid, NULL, 0);
    if (*semaphores == (HiveSemaphores*)-1) {
        perror("shmat (semaforów)");
        return NULL;
    }

    return gBeekeeperArgs->hive;
}

void cleanup(int signum) {
    (void)signum; // Oznacz jako nieużywany

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    // Zwolnij pamięć współdzieloną dla HiveData
    if (shmctl(gBeekeeperArgs->shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID (HiveData)");
    } 

    // Zwolnij pamięć współdzieloną dla semaforów
    if (shmctl(gBeekeeperArgs->semid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID (semaforów)");
    } 

    shmdt(semaphores);
    exit(EXIT_SUCCESS);
}

void handleSignalAddFrames(int signum) {
    (void)signum; // Oznacz jako nieużywany

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    sem_wait(&semaphores->hiveSem);

    hive->N = 2 * hive->N;
    printf("[Pszczelarz - sygnał] Dodano ramki. N = %d\n", hive->N);

    sem_post(&semaphores->hiveSem);

    shmdt(semaphores);
}

void handleSignalRemoveFrames(int signum) {
    (void)signum; // Oznacz jako nieużywany

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    sem_wait(&semaphores->hiveSem);

    hive->N /= 2;
    printf("[Pszczelarz - sygnał] Usunięto ramki. N = %d\n", hive->N);

    sem_post(&semaphores->hiveSem);

    shmdt(semaphores);
}

void beekeeperWorker(BeekeeperArgs* arg) {
    gBeekeeperArgs = arg;

    // Rejestracja obsługi sygnałów
    struct sigaction sa1;
    memset(&sa1, 0, sizeof(sa1));
    sa1.sa_handler = handleSignalAddFrames;
    if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
        perror("[Pszczelarz] sigaction(SIGUSR1)");
    }

    struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = handleSignalRemoveFrames;
    if (sigaction(SIGUSR2, &sa2, NULL) == -1) {
        perror("[Pszczelarz] sigaction(SIGUSR2)");
    }

    // Rejestracja obsługi sygnału SIGINT (Ctrl+C)
    struct sigaction sa3;
    memset(&sa3, 0, sizeof(sa3));
    sa3.sa_handler = cleanup;
    if (sigaction(SIGINT, &sa3, NULL) == -1) {
        perror("[Pszczelarz] sigaction(SIGINT)");
    }

    // Pszczelarz działa w tle
    while (1) {
        sleep(1);
    }
}
#include "beekeeper.h"
#include <string.h>
#include <signal.h>
#include <common.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/prctl.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

// Funkcja pomocnicza do uzyskania wskaźnika na HiveData i semafory
HiveData* getHiveDataAndSemaphores(HiveSemaphores** semaphores) {
    if (gBeekeeperArgs == NULL) {
        return NULL;
    }

    *semaphores = gBeekeeperArgs->semaphores;
    return gBeekeeperArgs->hive;
}

void handleSignalAddFrames(int signum) {
    (void)signum;

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    if (sem_wait(&semaphores->hiveSem) == -1) {
        handleError("[Pszczelarz] sem_wait (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    hive->N = 2 * hive->N;
    logMessage(LOG_INFO, "[Pszczelarz - sygnał] Dodano ramki. N = %d", hive->N);

    if (sem_post(&semaphores->hiveSem) == -1) {
        handleError("[Pszczelarz] sem_post (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }
}

void handleSignalRemoveFrames(int signum) {
    (void)signum;

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    if (sem_wait(&semaphores->hiveSem) == -1) {
        handleError("[Pszczelarz] sem_wait (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    hive->N /= 2;
    logMessage(LOG_INFO, "[Pszczelarz - sygnał] Usunięto ramki. N = %d", hive->N);

    if (sem_post(&semaphores->hiveSem) == -1) {
        handleError("[Pszczelarz] sem_post (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }
}

void cleanup(int signum) {
    (void)signum;

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    if (shmctl(gBeekeeperArgs->shmid, IPC_RMID, NULL) == -1) {
        handleError("[Pszczelarz] shmctl IPC_RMID (HiveData)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    if (shmctl(gBeekeeperArgs->semid, IPC_RMID, NULL) == -1) {
        handleError("[Pszczelarz] shmctl IPC_RMID (semaforów)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    detachSharedMemory(semaphores);
    detachSharedMemory(hive);
    exit(EXIT_SUCCESS);
}

void beekeeperWorker(BeekeeperArgs* arg) {
    gBeekeeperArgs = arg;
    prctl(PR_SET_NAME, "beekeeper");

    // Dołącz pamięć współdzieloną tylko raz
    gBeekeeperArgs->hive = (HiveData*)attachSharedMemory(gBeekeeperArgs->shmid);
    gBeekeeperArgs->semaphores = (HiveSemaphores*)attachSharedMemory(gBeekeeperArgs->semid);
    if (gBeekeeperArgs->hive == NULL || gBeekeeperArgs->semaphores == NULL) {
        handleError("[Pszczelarz] attachSharedMemory", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    // Rejestracja obsługi sygnałów
    struct sigaction sa1;
    memset(&sa1, 0, sizeof(sa1));
    sa1.sa_handler = handleSignalAddFrames;
    if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
        handleError("[Pszczelarz] sigaction(SIGUSR1)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = handleSignalRemoveFrames;
    if (sigaction(SIGUSR2, &sa2, NULL) == -1) {
        handleError("[Pszczelarz] sigaction(SIGUSR2)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    // Rejestracja obsługi sygnału SIGINT (Ctrl+C)
    struct sigaction sa3;
    memset(&sa3, 0, sizeof(sa3));
    sa3.sa_handler = cleanup;
    if (sigaction(SIGINT, &sa3, NULL) == -1) {
        handleError("[Pszczelarz] sigaction(SIGINT)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    // Pszczelarz działa w tle
    while (1) {
        sleep(1);
    }

    detachSharedMemory(gBeekeeperArgs->hive);
    detachSharedMemory(gBeekeeperArgs->semaphores);
    exit(EXIT_SUCCESS);
}
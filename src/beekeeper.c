#include "beekeeper.h"
#include <string.h>
#include <signal.h>
#include <common.h>
#include <semaphore.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

void handleSignalAddFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;
    HiveSemaphores* semaphores = (HiveSemaphores*)shmat(gBeekeeperArgs->semid, NULL, 0); // Użyj gBeekeeperArgs->semid

    sem_wait(&semaphores->hiveSem);

    hive->N = 2 * hive->N;
    printf("[Pszczelarz - sygnał] Dodano ramki. N = %d\n", hive->N);

    sem_post(&semaphores->hiveSem);

    shmdt(semaphores);
}

void handleSignalRemoveFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;
    HiveSemaphores* semaphores = (HiveSemaphores*)shmat(gBeekeeperArgs->semid, NULL, 0); // Użyj gBeekeeperArgs->semid

    sem_wait(&semaphores->hiveSem);

    hive->N /= 2;
    printf("[Pszczelarz - sygnał] Usunięto ramki. N = %d\n", hive->N);

    sem_post(&semaphores->hiveSem);

    shmdt(semaphores);
}

void beekeeperWorker(BeekeeperArgs* arg) {
    gBeekeeperArgs = arg;

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

    while (1) {
        sleep(1); // Pszczelarz działa w tle
    }

    exit(EXIT_SUCCESS);
}
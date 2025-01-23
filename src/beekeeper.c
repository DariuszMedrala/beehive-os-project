#include "beekeeper.h"
#include <string.h>
#include <signal.h>
#include <common.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

void handleSignalAddFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_lock");
        logMessage("[Pszczelarz - sygnał] Error locking hiveMutex");
        return;
    }

    hive->N = 2 * hive->N; // Zwiększamy do 2*N
    printf("[Pszczelarz - sygnał] Dodano ramki. N = %d\n", hive->N);
    logMessage("[Pszczelarz - sygnał] SIGUSR1 received: Added frames. N = %d", hive->N);

    if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_unlock");
        logMessage("[Pszczelarz - sygnał] Error unlocking hiveMutex");
        return;
    }
}

void handleSignalRemoveFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_lock");
        logMessage("[Pszczelarz - sygnał] Error locking hiveMutex");
        return;
    }

    hive->N /= 2; // Zmniejszamy o 50%
    logMessage("[Pszczelarz - sygnał] SIGUSR2 received: Removed frames. N = %d", hive->N);
    
    printf("[Pszczelarz - sygnał] Usunięto ramki. N = %d\n", hive->N);

    if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_unlock");
        logMessage("[Pszczelarz - sygnał] Error unlocking hiveMutex");
        return;
    }
}

void* beekeeperWorker(void* arg) {
    gBeekeeperArgs = (BeekeeperArgs*)arg;

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


    pthread_exit(NULL);
}

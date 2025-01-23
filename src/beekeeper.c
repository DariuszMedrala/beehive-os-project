#include "beekeeper.h"
#include <string.h>
#include <signal.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

void handleSignalAddFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_lock");
        return;
    }

    hive->N = 2 * hive->N; // Zwiększamy do 2*N
    printf("[Pszczelarz - sygnał] Dodano ramki. N = %d\n", hive->N);

    if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_unlock");
        return;
    }
}

void handleSignalRemoveFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_lock");
        return;
    }

    hive->N /= 2; // Zmniejszamy o 50%
    
    printf("[Pszczelarz - sygnał] Usunięto ramki. N = %d\n", hive->N);

    if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
        perror("[Pszczelarz - sygnał] pthread_mutex_unlock");
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

    while (1) {
        sleep(30);
        printf("[Pszczelarz] Czekam na sygnały (co 5 sek)...\n");
    }

    pthread_exit(NULL);
}

#include "beekeeper.h"
#include <string.h>
#include <signal.h>
#include "common.h"
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/prctl.h>

/**
 * Global pointer to BeekeeperArgs, used for signal handling.
 * Initialized when the beekeeper process starts.
 */
static BeekeeperArgs* gBeekeeperArgs = NULL;

/**
 * Helper function to retrieve hive data and semaphores for signal handling.
 * Ensures the beekeeper process can safely modify the hive state in response to signals.
 *
 * @param semaphores Pointer to store the semaphore structure reference.
 * @return A pointer to the hive data structure, or NULL if gBeekeeperArgs is not initialized.
 */
HiveData* getHiveDataAndSemaphores(HiveSemaphores** semaphores) {
    if (gBeekeeperArgs == NULL) {
        logMessage(LOG_WARNING, "[Beekeeper] gBeekeeperArgs is NULL during signal handling.");
        return NULL;
    }

    *semaphores = gBeekeeperArgs->semaphores;
    return gBeekeeperArgs->hive;
}

/**
 * Signal handler to add frames to the hive.
 * Doubles the hive's capacity (N) when SIGUSR1 is received.
 * Includes error handling for semaphore operations.
 *
 * @param signum Signal number (unused).
 */
void handleSignalAddFrames(int signum) {
    (void)signum; // Unused parameter
    logMessage(LOG_INFO, "[Beekeeper] Received SIGUSR1 signal.");

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    if (sem_wait(&semaphores->hiveSem) == -1) {
        handleError("[Beekeeper] sem_wait (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    if (hive->N * 2 > MAX_BEES) {
        hive->N = MAX_BEES;
        logMessage(LOG_WARNING, "[Beekeeper - Signal] Hive size capped at MAXBEES = %d", MAX_BEES);
    } else {
        hive->N *= 2;
        logMessage(LOG_INFO, "[Beekeeper - Signal] Added frames. New N = %d", hive->N);
    }

    if (sem_post(&semaphores->hiveSem) == -1) {
        handleError("[Beekeeper] sem_post (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }
}

/**
 * Signal handler to remove frames from the hive.
 * Halves the hive's capacity (N) when SIGUSR2 is received.
 * Includes error handling for semaphore operations.
 *
 * @param signum Signal number (unused).
 */
void handleSignalRemoveFrames(int signum) {
    (void)signum; // Unused parameter

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    if (sem_wait(&semaphores->hiveSem) == -1) {
        handleError("[Beekeeper] sem_wait (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    hive->N /= 2; // Halve the hive size
    logMessage(LOG_INFO, "[Beekeeper - Signal] Removed frames. New N = %d", hive->N);

    if (sem_post(&semaphores->hiveSem) == -1) {
        handleError("[Beekeeper] sem_post (hiveSem)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }
}

/**
 * Cleanup function to release resources and terminate the beekeeper process.
 * Triggered by SIGINT (e.g., Ctrl+C).
 * Includes error handling for resource cleanup operations.
 *
 * @param signum Signal number (unused).
 */
void cleanup(int signum) {
    (void)signum; // Unused parameter

    HiveSemaphores* semaphores;
    HiveData* hive = getHiveDataAndSemaphores(&semaphores);
    if (hive == NULL) return;

    // Attempt to release shared memory and semaphores; log warnings on failure
    if (shmctl(gBeekeeperArgs->shmid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[Beekeeper] Failed to remove shared memory for HiveData.");
    }

    if (shmctl(gBeekeeperArgs->semid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[Beekeeper] Failed to remove shared memory for semaphores.");
    }

    detachSharedMemory(semaphores);
    detachSharedMemory(hive);
    logMessage(LOG_INFO, "[Beekeeper] Cleanup complete. Exiting process.");
    exit(EXIT_SUCCESS);
}

/**
 * beekeeperWorker:
 * Implements the main behavior of the beekeeper process.
 * Includes detailed error handling for memory and semaphore operations.
 *
 * Detailed functionality:
 * 1. Attaches to shared memory for hive data and semaphores.
 * 2. Sets up signal handlers for dynamic hive management (SIGUSR1, SIGUSR2, SIGINT).
 * 3. Waits in an infinite loop to handle incoming signals.
 * 4. Cleans up shared resources upon termination.
 *
 * @param arg Pointer to BeekeeperArgs containing shared memory and semaphore details.
 */
void beekeeperWorker(BeekeeperArgs* arg) {
    gBeekeeperArgs = arg;
    prctl(PR_SET_NAME, "beekeeper");

    // Attach to shared memory for hive data and semaphores
    gBeekeeperArgs->hive = (HiveData*)attachSharedMemory(gBeekeeperArgs->shmid);
    gBeekeeperArgs->semaphores = (HiveSemaphores*)attachSharedMemory(gBeekeeperArgs->semid);
    if (gBeekeeperArgs->hive == NULL || gBeekeeperArgs->semaphores == NULL) {
        handleError("[Beekeeper] attachSharedMemory", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    // Register signal handlers
    struct sigaction sa1 = {0};
    sa1.sa_handler = handleSignalAddFrames;
    if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
        handleError("[Beekeeper] sigaction(SIGUSR1)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    struct sigaction sa2 = {0};
    sa2.sa_handler = handleSignalRemoveFrames;
    if (sigaction(SIGUSR2, &sa2, NULL) == -1) {
        handleError("[Beekeeper] sigaction(SIGUSR2)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    struct sigaction sa3 = {0};
    sa3.sa_handler = cleanup;
    if (sigaction(SIGINT, &sa3, NULL) == -1) {
        handleError("[Beekeeper] sigaction(SIGINT)", gBeekeeperArgs->shmid, gBeekeeperArgs->semid);
    }

    logMessage(LOG_INFO, "[Beekeeper] Process started and waiting for signals.");

    // Infinite loop to keep the beekeeper process running
    while (1) {
        sleep(1); // Sleep to reduce CPU usage
    }

    detachSharedMemory(gBeekeeperArgs->hive);
    detachSharedMemory(gBeekeeperArgs->semaphores);
    exit(EXIT_SUCCESS);
}

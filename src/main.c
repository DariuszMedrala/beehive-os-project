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

/**
 * Initializes the hive data with default values.
 * Ensures that the hive is properly set up before simulation starts.
 *
 * @param hive Pointer to the HiveData structure to initialize.
 * @param N Initial size of the hive (number of frames).
 */
void initHiveData(HiveData* hive, int N) {
    hive->currentBeesInHive = 0;
    hive->N = N;
    hive->beesAlive = N;
}

/**
 * Main entry point of the hive simulation program.
 *
 * Detailed functionality:
 * 1. Validates command-line arguments for hive size (N), queen's egg-laying interval (T_k), and egg count per cycle.
 * 2. Creates shared memory segments for hive data and semaphores.
 * 3. Initializes hive data and semaphores.
 * 4. Spawns the queen, beekeeper, and initial bee processes.
 * 5. Waits for all bee processes to complete and cleans up resources.
 *
 * Includes error handling for shared memory, semaphore initialization, and process creation.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, or 1 on failure.
 */
int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <N: initial number of hive frames> <T_k: queen egg-laying interval> <eggsCount: eggs per cycle>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int T_k = atoi(argv[2]);
    int eggsCount = atoi(argv[3]);

    if (N <= 0 || T_k <= 0 || eggsCount <= 0) {
        fprintf(stderr, "Error: All arguments must be positive integers.\n");
        return 1;
    }

    // Ensure the number of initial bees does not exceed MAX_BEES
    if (N > MAX_BEES) {
        logMessage(LOG_WARNING, "[MAIN] Initial hive size (%d) exceeds MAX_BEES (%d). Setting N to %d.", N, MAX_BEES, MAX_BEES);
        N = MAX_BEES;
    }

    // Create shared memory for hive data
    int shmid = shmget(IPC_PRIVATE, sizeof(HiveData), IPC_CREAT | 0666);
    if (shmid == -1) {
        handleError("[MAIN] Failed to create shared memory for HiveData", -1, -1);
    }

    HiveData* hive = (HiveData*)attachSharedMemory(shmid);
    if (hive == NULL) {
        handleError("[MAIN] Failed to attach shared memory for HiveData", shmid, -1);
    }

    initHiveData(hive, N);

    // Create shared memory for semaphores
    int semid = shmget(IPC_PRIVATE, sizeof(HiveSemaphores), IPC_CREAT | 0666);
    if (semid == -1) {
        handleError("[MAIN] Failed to create shared memory for semaphores", shmid, -1);
    }

    HiveSemaphores* semaphores = (HiveSemaphores*)attachSharedMemory(semid);
    if (semaphores == NULL) {
        handleError("[MAIN] Failed to attach shared memory for semaphores", shmid, semid);
    }

    // Initialize semaphores
    if (sem_init(&semaphores->hiveSem, 1, 1) == -1) {
        handleError("[MAIN] Failed to initialize hiveSem", shmid, semid);
    }

    for (int i = 0; i < 2; i++) {
        if (sem_init(&semaphores->entranceSem[i], 1, 1) == -1) {
            handleError("[MAIN] Failed to initialize entranceSem", shmid, semid);
        }

        if (sem_init(&semaphores->fifoQueue[i], 1, 1) == -1) { // Initialize FIFO queue semaphores
            handleError("[MAIN] Failed to initialize fifoQueue", shmid, semid);
        }
    }

    // Spawn the queen process
    pid_t queenPid = fork();
    if (queenPid == 0) {
        QueenArgs queenArgs = {T_k, eggsCount, hive, semaphores, semid, shmid};
        queenWorker(&queenArgs);
        exit(EXIT_SUCCESS);
    } else if (queenPid < 0) {
        handleError("[MAIN] Failed to fork queen process", shmid, semid);
    }

    // Spawn the beekeeper process
    pid_t beekeeperPid = fork();
    if (beekeeperPid == 0) {
        BeekeeperArgs keeperArgs = {hive, semaphores, semid, shmid};
        beekeeperWorker(&keeperArgs);
        exit(EXIT_SUCCESS);
    } else if (beekeeperPid < 0) {
        handleError("[MAIN] Failed to fork beekeeper process", shmid, semid);
    }

    // Spawn initial bee processes
    for (int i = 0; i < N; i++) {
        pid_t beePid = fork();
        if (beePid == 0) {
            BeeArgs beeArgs = {i, 0, MAX_BEE_VISITS, T_IN_HIVE, hive, semaphores, false, semid, shmid};
            beeWorker(&beeArgs);
            exit(EXIT_SUCCESS);
        } else if (beePid < 0) {
            handleError("[MAIN] Failed to fork bee process", shmid, semid);
        }
    }

    // Wait for all bee processes to finish
    for (int i = 0; i < N; i++) {
        if (wait(NULL) == -1) {
            handleError("[MAIN] Failed to wait for bee processes", shmid, semid);
        }
    }

    // Terminate the queen and beekeeper processes
    if (kill(queenPid, SIGTERM) == -1) {
        handleError("[MAIN] Failed to terminate queen process", shmid, semid);
    }
    if (kill(beekeeperPid, SIGTERM) == -1) {
        handleError("[MAIN] Failed to terminate beekeeper process", shmid, semid);
    }

    // Cleanup shared memory and semaphores
    detachSharedMemory(hive); // Errors will be logged internally
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[MAIN] Failed to remove shared memory for HiveData.");
    }

    detachSharedMemory(semaphores); // Errors will be logged internally
    if (shmctl(semid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[MAIN] Failed to remove shared memory for semaphores.");
    }

    logMessage(LOG_INFO, "[MAIN] Simulation completed successfully.");
    return 0;
}

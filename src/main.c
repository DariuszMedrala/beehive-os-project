#include "common.h"
#include "bee.h"
#include "queen.h"
#include "beekeeper.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Main entry point of the hive simulation program.
 *
 * Detailed functionality:
 * 1. Validates command-line arguments for hive size (N), queen's egg-laying interval (T_k), and egg count per cycle.
 * 2. Uses modularized initialization functions to set up shared memory and semaphores.
 * 3. Spawns the queen, beekeeper, and initial bee processes.
 * 4. Waits for all bee processes to complete and cleans up resources.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, or 1 on failure.
 */
int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <N: initial hive size> <T_k: egg-laying interval> <eggsCount>\n", argv[0]);
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

    // Shared memory IDs
    int shmid, semid;

    // Initialize HiveData and HiveSemaphores using modular functions
    HiveData* hive = initHiveData(N, &shmid);
    HiveSemaphores* semaphores = initHiveSemaphores(&semid);

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
    while (1) {
        pid_t result = wait(NULL);
        if (result == -1) {
            if (errno == ECHILD) {
                // No more child processes
                break;
            } else {
                handleError("[MAIN] Failed to wait for bee processes", shmid, semid);
            }
        }
    }

    // Terminate the queen and beekeeper processes
    if (kill(queenPid, SIGTERM) == -1) {
        handleError("[MAIN] Failed to terminate queen process", shmid, semid);
    }
    if (waitpid(queenPid, NULL, 0) == -1) {
        handleError("[MAIN] Failed to wait for queen process", shmid, semid);
    }

    if (kill(beekeeperPid, SIGTERM) == -1) {
        handleError("[MAIN] Failed to terminate beekeeper process", shmid, semid);
    }
    if (waitpid(beekeeperPid, NULL, 0) == -1) {
        handleError("[MAIN] Failed to wait for beekeeper process", shmid, semid);
    }

    // Cleanup shared memory and semaphores
    cleanupResources(shmid, semid);

    logMessage(LOG_INFO, "[MAIN] Simulation completed successfully.");
    return 0;
}

#ifndef BEE_H
#define BEE_H

#include "common.h"

/**
 * The BeeArgs struct contains all the necessary data for each bee process.
 * Each bee operates independently and interacts with the shared hive data
 * and synchronization mechanisms provided in this structure.
 */
typedef struct {
    int id;         ///< Unique ID of the bee.
    int visits;     ///< Number of visits the bee has made to the hive.
    int maxVisits;  ///< Maximum number of visits after which the bee dies.
    int T_inHive;   ///< Time (in seconds) the bee spends inside the hive during each visit.
    HiveData* hive; ///< Pointer to the shared memory structure representing the hive state.
    HiveSemaphores* semaphores; ///< Pointer to the shared semaphore structure for synchronization.
    bool startInHive; ///< Indicates whether the bee starts its life inside the hive.
    int semid;      ///< Shared memory identifier for semaphores.
    int shmid;      ///< Shared memory identifier for hive data.
} BeeArgs;

/**
 * beeWorker:
 * The main function executed by each bee process.
 * 
 * Detailed behavior:
 * - Attaches to shared memory for hive data and semaphores.
 * - Manages the bee's lifecycle, including entering and leaving the hive.
 * - Synchronizes hive access using semaphores to ensure proper concurrent behavior.
 * - Logs relevant events such as entering, exiting, and dying.
 * - Cleans up shared memory attachments before termination.
 * 
 * @param arg A pointer to a BeeArgs structure containing the bee's individual and shared parameters.
 */
void beeWorker(BeeArgs* arg);

#endif

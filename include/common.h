#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <stdarg.h>

/**
 * Maximum allowed bees in the hive.
 * This constant is used to limit the number of bees that can exist simultaneously.
 */
#define MAX_BEES 1000

/**
 * Time (in seconds) a bee spends inside the hive during each visit.
 */
#define T_IN_HIVE 2

/**
 * Maximum number of visits a bee can make before it dies.
 */
#define MAX_BEE_VISITS 3

/**
 * Minimum time (in seconds) a bee waits before entering the hive.
 */
#define MIN_WAIT_TIME 0.5

/**
 * Maximum time (in seconds) a bee waits before entering the hive.
 */
#define MAX_WAIT_TIME 1.0

/**
 * Minimum time (in seconds) a bee spends outside the hive.
 */
#define MIN_OUTSIDE_TIME 1

/**
 * Maximum time (in seconds) a bee spends outside the hive.
 */
#define MAX_OUTSIDE_TIME 3
/**
 * Console color codes for pretty-printed messages.
 * These can be used to differentiate log levels when printing to the terminal.
 */
#define RESET "\033[0m"  // Resets the console color to default.
#define RED "\033[31m"   // Used for error messages.
#define GREEN "\033[32m" // Used for informational messages.
#define YELLOW "\033[33m" // Used for warnings.
#define BLUE "\033[34m"  // Used for debug messages.

/**
 * Enum representing the levels of logging.
 * Used for filtering log messages based on their severity.
 */
typedef enum {
    LOG_DEBUG,   // Debug-level messages, typically for developers.
    LOG_INFO,    // Informational messages about normal operations.
    LOG_WARNING, // Warning messages indicating potential issues.
    LOG_ERROR    // Error messages indicating critical failures.
} LogLevel;

/**
 * Struct to configure logging options.
 * Provides options to enable/disable console and file logging, and set the minimum log levels.
 */
typedef struct {
    bool logToConsole;         // Whether to log messages to the console.
    bool logToFile;            // Whether to log messages to a file.
    LogLevel consoleLogLevel;  // Minimum log level for console messages.
    LogLevel fileLogLevel;     // Minimum log level for file messages.
} LogConfig;

/**
 * Struct representing the global state of the hive.
 * Tracks the number of bees in the hive and overall colony health.
 */
typedef struct {
    int currentBeesInHive;  // Current number of bees inside the hive.
    int N;                  // Initial size of the hive (number of frames).
    int beesAlive;          // Total number of live bees in the colony.
} HiveData;

/**
 * Struct for hive synchronization primitives.
 * Includes semaphores for controlling access to hive operations.
 */
typedef struct {
    sem_t hiveSem;          // Semaphore for general hive access control.
    sem_t entranceSem[2];   // Semaphores for each hive entrance.
    sem_t fifoQueue[2];     // FIFO queue semaphores for each entrance.
} HiveSemaphores;

/**
 * Global shared memory identifiers.
 * Used to track and release shared resources at the end of the program.
 */
extern int shmid;  // Shared memory identifier for HiveData.
extern int semid;  // Shared memory identifier for HiveSemaphores.

/**
 * Global logging configuration.
 * This variable determines the logging behavior throughout the program.
 */
extern LogConfig logConfig;

/**
 * Logs a formatted message to the console and/or file based on the log configuration.
 *
 * @param level The severity level of the message.
 * @param format The formatted string to log, followed by optional arguments.
 */
void logMessage(LogLevel level, const char* format, ...);

/**
 * Initializes shared memory for hive data and returns a pointer to it.
 * @param N Initial hive size (number of frames).
 * @param shmid Pointer to store the shared memory ID.
 * @return Pointer to initialized HiveData.
 */
HiveData* initHiveData(int N, int* shmid);


/**
 * Initializes shared memory for semaphores and returns a pointer to it.
 * @param semid Pointer to store the shared memory ID.
 * @return Pointer to initialized HiveSemaphores.
 */
HiveSemaphores* initHiveSemaphores(int* semid);

/**
 * Cleans up shared memory and semaphores.
 * @param shmid Shared memory ID for HiveData.
 * @param semid Shared memory ID for HiveSemaphores.
 */
void cleanupResources(int shmid, int semid);

/**
 * Attaches to a shared memory segment.
 *
 * @param shmid The shared memory identifier.
 * @return A pointer to the attached shared memory segment, or NULL on failure.
 */
void* attachSharedMemory(int shmid);

/**
 * Detaches from a shared memory segment.
 *
 * @param sharedMemory A pointer to the shared memory segment to detach.
 */
void detachSharedMemory(void* sharedMemory);

/**
 * Handles errors by logging the message, releasing shared resources, and terminating the program.
 *
 * @param message A descriptive error message.
 * @param shmid The shared memory identifier to release (if valid).
 * @param semid The semaphore memory identifier to release (if valid).
 */
void handleError(const char* message, int shmid, int semid);

/**
 * calculateP:
 * Calculates the maximum number of bees that can fit inside the hive at any given time.
 * 
 * @param N The initial size of the hive (number of frames).
 * @return The maximum number of bees allowed in the hive.
 */
int calculateP(int N);




#endif

#include "common.h"

// Global shared memory identifiers, initialized to invalid values (-1)
int shmid = -1;  ///< Shared memory identifier for HiveData.
int semid = -1;  ///< Shared memory identifier for HiveSemaphores.

// Default logging configuration
LogConfig logConfig = {
    .logToConsole = true,    ///< Enable logging to the console.
    .logToFile = true,       ///< Enable logging to a file.
    .consoleLogLevel = LOG_DEBUG, ///< Log all levels to the console.
    .fileLogLevel = LOG_DEBUG     ///< Log all levels to the file.
};



HiveData* initHiveData(int N, int* shmid) {
    *shmid = shmget(IPC_PRIVATE, sizeof(HiveData), IPC_CREAT | 0666);
    if (*shmid == -1) {
        handleError("[INIT] Failed to create shared memory for HiveData", -1, -1);
    }

    HiveData* hive = (HiveData*)attachSharedMemory(*shmid);
    if (hive == NULL) {
        handleError("[INIT] Failed to attach shared memory for HiveData", *shmid, -1);
    }

    hive->currentBeesInHive = 0;
    hive->N = N;
    hive->beesAlive = N;
    return hive;
}

/**
 * calculateP:
 * Calculates the maximum number of bees that can fit inside the hive at any given time.
 * Includes validation to ensure N is valid.
 * 
 * @param N The initial size of the hive (number of frames).
 * @return The maximum number of bees allowed in the hive.
 */
int calculateP(int N) {
    if (N <= 0) {
        logMessage(LOG_ERROR, "[calculateP] Invalid hive size N = %d. Must be greater than 0.", N);
        return 0;
    }
    return (N / 2) - 1;
}

/**
 * logMessage:
 * Logs a formatted message to the console and/or file, depending on the global log configuration.
 * Includes error handling for file operations.
 * 
 * @param level The severity level of the message.
 * @param format The formatted string, followed by optional arguments.
 */
void logMessage(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    const char* levelStr;
    const char* color;
    switch (level) {
        case LOG_DEBUG:
            levelStr = "DEBUG";
            color = BLUE;
            break;
        case LOG_INFO:
            levelStr = "INFO";
            color = GREEN;
            break;
        case LOG_WARNING:
            levelStr = "WARNING";
            color = YELLOW;
            break;
        case LOG_ERROR:
            levelStr = "ERROR";
            color = RED;
            break;
        default:
            levelStr = "UNKNOWN";
            color = RESET;
            break;
    }

    // Prepare the common log message
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    // Log to console if enabled and level meets the threshold
    if (logConfig.logToConsole && level >= logConfig.consoleLogLevel) {
        printf("%s[%s] %s%s\n", color, levelStr, buffer, RESET);
    }

    // Log to file if enabled and level meets the threshold
    if (logConfig.logToFile && level >= logConfig.fileLogLevel) {
        FILE* logFile = fopen("beehive.log", "a");
        if (!logFile) {
            // If the log file cannot be opened, print an error to stderr
            perror("[logMessage] Failed to open log file");
        } else {
            // Get the current time and format the timestamp
            time_t now = time(NULL);
            struct tm* t = localtime(&now);
            if (t) {
                // Write the log message with a timestamp
                fprintf(logFile, "[%02d-%02d-%04d %02d:%02d:%02d] [%s] %s\n",
                        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
                        t->tm_hour, t->tm_min, t->tm_sec, levelStr, buffer);
            } else {
                // If time retrieval fails, log without a timestamp
                fprintf(logFile, "[Time unavailable] [%s] %s\n", levelStr, buffer);
            }
            fclose(logFile); // Close the log file
        }
    }

    va_end(args);
}

/**
 * attachSharedMemory:
 * Attaches to a shared memory segment and returns a pointer to it.
 * If the attachment fails, logs an error and exits the program.
 * 
 * @param shmid The shared memory identifier.
 * @return A pointer to the shared memory segment.
 */
void* attachSharedMemory(int shmid) {
    void* sharedMemory = shmat(shmid, NULL, 0);
    if (sharedMemory == (void*)-1) {
        handleError("[attachSharedMemory] shmat failed", shmid, -1);
    }
    return sharedMemory;
}

/**
 * detachSharedMemory:
 * Detaches from a shared memory segment.
 * If the detachment fails, logs an error.
 * 
 * @param sharedMemory A pointer to the shared memory segment to detach.
 */
void detachSharedMemory(void* sharedMemory) {
    if (shmdt(sharedMemory) == -1) {
        handleError("[detachSharedMemory] shmdt failed", -1, -1);
    }
}

HiveSemaphores* initHiveSemaphores(int* semid) {
    *semid = shmget(IPC_PRIVATE, sizeof(HiveSemaphores), IPC_CREAT | 0666);
    if (*semid == -1) {
        handleError("[INIT] Failed to create shared memory for HiveSemaphores", -1, -1);
    }

    HiveSemaphores* semaphores = (HiveSemaphores*)attachSharedMemory(*semid);
    if (semaphores == NULL) {
        handleError("[INIT] Failed to attach shared memory for HiveSemaphores", -1, *semid);
    }

    // Initialize semaphores
    if (sem_init(&semaphores->hiveSem, 1, 1) == -1) {
        handleError("[INIT] Failed to initialize hiveSem", -1, *semid);
    }

    for (int i = 0; i < 2; i++) {
        if (sem_init(&semaphores->entranceSem[i], 1, 1) == -1) {
            handleError("[INIT] Failed to initialize entranceSem", -1, *semid);
        }
        if (sem_init(&semaphores->fifoQueue[i], 1, 1) == -1) {
            handleError("[INIT] Failed to initialize fifoQueue", -1, *semid);
        }
    }
    return semaphores;
}

void cleanupResources(int shmid, int semid) {
    // Detach and remove shared memory for HiveData
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[CLEANUP] Failed to remove shared memory for HiveData.");
    }

    // Detach and remove shared memory for semaphores
    if (shmctl(semid, IPC_RMID, NULL) == -1) {
        logMessage(LOG_WARNING, "[CLEANUP] Failed to remove shared memory for semaphores.");
    }
}

/**
 * handleError:
 * Handles critical errors by logging the message, printing the system error message,
 * releasing shared resources, and terminating the program.
 * 
 * @param message A descriptive error message.
 * @param shmid The shared memory identifier to release (if valid).
 * @param semid The semaphore memory identifier to release (if valid).
 */
void handleError(const char* message, int shmid, int semid) {
    // Log the error message with the system error
    logMessage(LOG_ERROR, "%s: %s", message, strerror(errno));
    
    // Print the system error message to stderr
    perror(message);

    // Release shared memory if shmid is valid
    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            logMessage(LOG_ERROR, "[handleError] Failed to remove shared memory (shmid: %d): %s", shmid, strerror(errno));
            perror("shmctl IPC_RMID (shared memory)");
        }
    }

    // Release semaphores if semid is valid
    if (semid != -1) {
        if (shmctl(semid, IPC_RMID, NULL) == -1) {
            logMessage(LOG_ERROR, "[handleError] Failed to remove semaphores (semid: %d): %s", semid, strerror(errno));
            perror("shmctl IPC_RMID (semaphores)");
        }
    }

    exit(EXIT_FAILURE);
 
}

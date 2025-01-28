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

    // Log to console if enabled and level meets the threshold
    if (logConfig.logToConsole && level >= logConfig.consoleLogLevel) {
        printf("%s[%s] ", color, levelStr);
        vprintf(format, args);
        printf("%s\n", RESET);
    }

    // Log to file if enabled and level meets the threshold
    if (logConfig.logToFile && level >= logConfig.fileLogLevel) {
        FILE* logFile = fopen("beehive.log", "w");
        if (!logFile) {
            perror("[logMessage] Failed to open log file");
            return;
        }

        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        if (!t) {
            handleError("[logMessage] localtime failed", -1, -1);
        }
        fprintf(logFile, "[%02d-%02d-%04d %02d:%02d:%02d] [%s] ",
                t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
                t->tm_hour, t->tm_min, t->tm_sec, levelStr);
        vfprintf(logFile, format, args);
        fprintf(logFile, "\n");
        fclose(logFile);
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

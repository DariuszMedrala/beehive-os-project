#include "common.h"

// Definicje zmiennych globalnych
int shmid = -1;  // Identyfikator pamięci współdzielonej dla HiveData
int semid = -1;  // Identyfikator pamięci współdzielonej dla semaforów

// Konfiguracja logowania (domyślne wartości)
LogConfig logConfig = {
    .logToConsole = true,   // Logowanie do konsoli włączone
    .logToFile = true,      // Logowanie do pliku włączone
    .consoleLogLevel = LOG_DEBUG,  // Wszystkie poziomy logowania w konsoli
    .fileLogLevel = LOG_DEBUG      // Wszystkie poziomy logowania w pliku
};

int calculateP(int N) {
    return (N / 2) - 1;
}

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

    // Logowanie do konsoli
    if (logConfig.logToConsole && level >= logConfig.consoleLogLevel) {
        printf("%s[%s] ", color, levelStr);
        vprintf(format, args);
        printf("%s\n", RESET);
    }

    // Logowanie do pliku
    if (logConfig.logToFile && level >= logConfig.fileLogLevel) {
        FILE* logFile = fopen("beehive.log", "a");
        if (!logFile) {
            handleError("[LOGGER] Failed to open log file", -1, -1);
            return;
        }

        // Dodanie znacznika czasu
        time_t now = time(NULL);
        if (now == -1) {
            handleError("[LOGGER] time", -1, -1);
        }
        struct tm* t = localtime(&now);
        fprintf(logFile, "[%02d-%02d-%04d %02d:%02d:%02d] [%s] ",
                t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
                t->tm_hour, t->tm_min, t->tm_sec, levelStr);

        // Zapisanie wiadomości logu
        vfprintf(logFile, format, args);
        fprintf(logFile, "\n");
        fclose(logFile);
    }

    va_end(args);
}

// Funkcja dołączająca pamięć współdzieloną
void* attachSharedMemory(int shmid) {
    void* sharedMemory = shmat(shmid, NULL, 0);
    if (sharedMemory == (void*)-1) {
        handleError("shmat", shmid, -1);
    }
    return sharedMemory;
}

// Funkcja odłączająca pamięć współdzieloną
void detachSharedMemory(void* sharedMemory) {
    if (shmdt(sharedMemory) == -1) {
        handleError("shmdt", -1, -1);
    }
}

// Funkcja do obsługi błędów
void handleError(const char* message, int shmid, int semid) {
    logMessage(LOG_ERROR, "%s: %s", message, strerror(errno));

    // Zwolnij pamięć współdzieloną, jeśli shmid jest prawidłowy
    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            logMessage(LOG_ERROR, "shmctl IPC_RMID (HiveData): %s", strerror(errno));
        }
    }

    // Zwolnij pamięć współdzieloną dla semaforów, jeśli semid jest prawidłowy
    if (semid != -1) {
        if (shmctl(semid, IPC_RMID, NULL) == -1) {
            logMessage(LOG_ERROR, "shmctl IPC_RMID (semaforów): %s", strerror(errno));
        }
    }

    exit(EXIT_FAILURE);
}
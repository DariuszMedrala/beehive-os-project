#include "common.h"
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

// Definicje zmiennych globalnych
int shmid = -1;  // Identyfikator pamięci współdzielonej dla HiveData
int semid = -1;  // Identyfikator pamięci współdzielonej dla semaforów

void coloredPrintf(const char* color, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("%s", color);
    vprintf(format, args);
    printf("%s", RESET);
    va_end(args);
}

void logMessage(const char* format, ...) {
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
    fprintf(logFile, "[%02d-%02d-%04d %02d:%02d:%02d] ",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec);

    // Zapisanie wiadomości logu
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fprintf(logFile, "\n");
    fclose(logFile);
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
    perror(message);

    // Zwolnij pamięć współdzieloną, jeśli shmid jest prawidłowy
    if (shmid != -1) {
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID (HiveData)");
        }
    }

    // Zwolnij pamięć współdzieloną dla semaforów, jeśli semid jest prawidłowy
    if (semid != -1) {
        if (shmctl(semid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID (semaforów)");
        }
    }

    exit(EXIT_FAILURE);
}
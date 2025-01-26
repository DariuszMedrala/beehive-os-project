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
        perror("[LOGGER] Failed to open log file");
        return;
    }

    // Dodanie znacznika czasu
    time_t now = time(NULL);
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
        perror("shmat");
        return NULL;
    }
    return sharedMemory;
}

// Funkcja odłączająca pamięć współdzieloną
void detachSharedMemory(void* sharedMemory) {
    if (shmdt(sharedMemory) == -1) {
        perror("shmdt");
    }
}

// Funkcja inicjalizująca semafory
void initSemaphores(HiveSemaphores* semaphores) {
    if (sem_init(&semaphores->hiveSem, 1, 1) == -1) {
        perror("sem_init (hiveSem)");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 2; i++) {
        if (sem_init(&semaphores->entranceSem[i], 1, 1) == -1) {
            perror("sem_init (entranceSem)");
            exit(EXIT_FAILURE);
        }
    }
}

// Funkcja niszcząca semafory
void destroySemaphores(HiveSemaphores* semaphores) {
    if (sem_destroy(&semaphores->hiveSem) == -1) {
        perror("sem_destroy (hiveSem)");
    }

    for (int i = 0; i < 2; i++) {
        if (sem_destroy(&semaphores->entranceSem[i]) == -1) {
            perror("sem_destroy (entranceSem)");
        }
    }
}
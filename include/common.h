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

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

#define MAX_BEES 1000

// Struktura przechowująca globalny stan ula
typedef struct {
    int currentBeesInHive;  // Aktualna liczba pszczół w ulu
    int N;                  // Początkowa liczba pszczół (rozmiar roju)
    int P;                  // Limit pszczół w ulu
    int beesAlive;          // Liczba żywych pszczół w całym roju
} HiveData;

// Semafory dla synchronizacji
typedef struct {
    sem_t hiveSem;          // Semafor dla dostępu do ula
    sem_t entranceSem[2];   // Semafor dla każdego wejścia
} HiveSemaphores;

// Zmienne globalne
extern int shmid;  // Identyfikator pamięci współdzielonej dla HiveData
extern int semid;  // Identyfikator pamięci współdzielonej dla semaforów

// Funkcje
void logMessage(const char* format, ...);
void coloredPrintf(const char* color, const char* format, ...);

// Funkcje pomocnicze do zarządzania pamięcią współdzieloną
void* attachSharedMemory(int shmid);
void detachSharedMemory(void* sharedMemory);

// Funkcja do obsługi błędów
void handleError(const char* message, int shmid, int semid);

#endif
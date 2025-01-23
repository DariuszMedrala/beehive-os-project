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

// Struktura przechowująca globalny stan ula
#define MAX_BEES 1000

typedef struct {
    int currentBeesInHive;  // Aktualna liczba pszczół w ulu (w danej chwili)       // Aktualna pojemność ula (ile pszczół może być w środku naraz)
    int N;                  // Początkowa liczba pszczół (rozmiar roju)
    int P;                   // Limit pszczół w ulu 
    bool entranceInUse[2];  // Czy dane wejście (0/1) jest w danym momencie używane

    pthread_mutex_t hiveMutex;// Mutex do synchronizacji dostępu do powyższych pól
    pthread_mutex_t entranceMutex[2]; // Mutexy dla każdego wejścia
    int beesAlive;         // Liczba żywych pszczół w całym roju (uwzględniając narodzone)
} HiveData;

#endif

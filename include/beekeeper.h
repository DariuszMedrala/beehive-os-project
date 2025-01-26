#ifndef BEEKEEPER_H
#define BEEKEEPER_H

#include "common.h"

typedef struct {
    HiveData* hive; // Wskaźnik na dane ula
    HiveSemaphores* semaphores; // Wskaźnik na semafory
    int semid;      // Identyfikator pamięci współdzielonej dla semaforów
    int shmid;      // Identyfikator pamięci współdzielonej dla HiveData
} BeekeeperArgs;

void beekeeperWorker(BeekeeperArgs* arg); // Funkcja dla procesu pszczelarza

#endif
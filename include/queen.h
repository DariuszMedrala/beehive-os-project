#ifndef QUEEN_H
#define QUEEN_H

#include "common.h"

typedef struct {
    int T_k;       // Co ile sekund królowa składa jaja
    int eggsCount; // Ile jaj składa naraz
    HiveData* hive; // Wskaźnik na dane ula
    HiveSemaphores* semaphores; // Wskaźnik na semafory
    int semid;     // Identyfikator pamięci współdzielonej dla semaforów
    int shmid;     // Identyfikator pamięci współdzielonej dla HiveData
} QueenArgs;

void queenWorker(QueenArgs* arg); // Funkcja dla procesu królowej

#endif
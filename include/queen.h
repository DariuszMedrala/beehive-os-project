#ifndef QUEEN_H
#define QUEEN_H

#include "common.h"

typedef struct {
    int T_k;       // Co ile sekund królowa składa jaja
    int eggsCount; // Ile jaj składa naraz
    HiveData* hive; // Wskaźnik na dane ula
    int semid;     // Identyfikator pamięci współdzielonej dla semaforów
} QueenArgs;

void queenWorker(QueenArgs* arg); // Funkcja dla procesu królowej

#endif
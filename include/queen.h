#ifndef QUEEN_H
#define QUEEN_H

#include "common.h"

typedef struct {
    int T_k;       // co ile sekund królowa składa jaja
    int eggsCount; // ile jaj składa naraz
    HiveData* hive;
} QueenArgs;

void* queenWorker(void* arg);

#endif

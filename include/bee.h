#ifndef BEE_H
#define BEE_H

#include "common.h"

typedef struct {
    int id;         // Unikalne ID pszczoły
    int visits;     // Ile razy pszczoła odwiedziła ul
    int maxVisits;  // Po ilu wizytach pszczoła umiera
    int T_inHive;   // Czas przebywania pszczoły w ulu (sekundy)
    HiveData* hive;
    bool startInHive;  // Czy pszczoła zaczyna życie w ulu
} BeeArgs;

void* beeWorker(void* arg);

#endif

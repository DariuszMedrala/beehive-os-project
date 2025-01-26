#ifndef BEE_H
#define BEE_H

#include "common.h"

typedef struct {
    int id;         // Unikalne ID pszczoły
    int visits;     // Ile razy pszczoła odwiedziła ul
    int maxVisits;  // Po ilu wizytach pszczoła umiera
    int T_inHive;   // Czas przebywania pszczoły w ulu (sekundy)
    HiveData* hive; // Wskaźnik na dane ula
    bool startInHive;  // Czy pszczoła zaczyna życie w ulu
    int semid;      // Identyfikator pamięci współdzielonej dla semaforów
} BeeArgs;

void beeWorker(BeeArgs* arg); // Funkcja dla procesu pszczoły

#endif
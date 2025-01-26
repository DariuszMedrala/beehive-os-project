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
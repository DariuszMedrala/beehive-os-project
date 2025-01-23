#ifndef BEEKEEPER_H
#define BEEKEEPER_H

#include "common.h"

typedef struct {
    HiveData* hive;
} BeekeeperArgs;

void* beekeeperWorker(void* arg);
void handleSignalAddFrames(int signum);
void handleSignalRemoveFrames(int signum);

#endif

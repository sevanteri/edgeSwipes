#ifndef TAPPER_H
#define TAPPER_H

#include <time.h>
#include <pthread.h>

typedef void* (*voidfunc)(void* arg);

typedef struct tapper_t {
    struct timespec start;
    struct timespec end;
    int count;
    pthread_t thread;
    pthread_attr_t thread_attr;
    int thread_alive;
    int delay;
    voidfunc func;
} tapper_t;


int
tapper_init(struct tapper_t* tapper, int delay, voidfunc func );

int
tapper_run(struct tapper_t*);

#endif

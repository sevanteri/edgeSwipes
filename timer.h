#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <pthread.h>

typedef void* (*voidfunc)(void* arg);

typedef struct taptimer_t {
    struct timespec start;
    struct timespec end;
    int count;
    pthread_t thread;
    pthread_attr_t thread_attr;
    int thread_alive;
    int delay;
    voidfunc func;
} taptimer_t;


int
timer_init(struct taptimer_t* timer, int delay, voidfunc func );

int
timer_run(struct taptimer_t*);

#endif

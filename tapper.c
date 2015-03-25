
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>
#include <unistd.h>

#include "tapper.h"

int
tapper_init(struct tapper_t* tapper, int delay, voidfunc func )
{
    pthread_attr_init(&(tapper->thread_attr));
    pthread_attr_setdetachstate(&(tapper->thread_attr), PTHREAD_CREATE_DETACHED);

    tapper->delay = delay;
    tapper->func = func;
    tapper->thread_alive = 0;
    tapper->count = 0;

    return 0;
}

static void*
func_wrapper(void* tapper)
{
    struct tapper_t *tim = (struct tapper_t*)tapper;
    tim->thread_alive = 1;
    const struct timespec del = {
        .tv_sec = 0,
        .tv_nsec = tim->delay
    };
    tim->count += 1;
    clock_nanosleep(CLOCK_REALTIME, 0, &del, NULL);
    tim->func(tapper);
    tim->count = 0;
    tim->thread_alive = 0;

    pthread_exit(NULL);
}

int
tapper_run(struct tapper_t* tapper)
{
    if (tapper->thread_alive)
        pthread_cancel(tapper->thread);

    int rc = pthread_create(
                            &(tapper->thread),
                            &(tapper->thread_attr),
                            func_wrapper,
                            (void*)tapper
                        );
    if (rc) {
        err(1, "Timer thread creation failed\n");
    }

    return 0;
}


int
tapper_destroy(struct tapper_t* tapper)
{
    pthread_attr_destroy(&(tapper->thread_attr));
    return 0;
}

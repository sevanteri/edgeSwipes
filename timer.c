
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>

#include "timer.h"

int
timer_init(struct taptimer_t* timer, int delay, voidfunc func )
{
    pthread_attr_init(&(timer->thread_attr));
    pthread_attr_setdetachstate(&(timer->thread_attr), PTHREAD_CREATE_DETACHED);
    /*pthread_attr_setstacksize(&(timer->thread_attr), 1600000);*/

    timer->delay = delay;
    timer->func = func;
    timer->thread_alive = 0;

    return 0;
}

static void*
func_wrapper(void* timer)
{
    struct taptimer_t *tim = (struct taptimer_t*)timer;
    tim->thread_alive = 1;
    const struct timespec del = {
        tim->delay
    };
    clock_nanosleep(CLOCK_REALTIME, 0, &del, NULL);
    tim->func(NULL);
    tim->thread_alive = 0;

    pthread_exit(NULL);
}

void* simplefun() {
    pthread_exit(NULL);
}

int
timer_run(struct taptimer_t* timer)
{
    if (timer->thread_alive)
        pthread_cancel(timer->thread);

    int rc = pthread_create(&(timer->thread), &(timer->thread_attr),
                        simplefun,
                        NULL);
    if (!rc) {

        err(1, "Timer thread creation failed\n");
    }

    return 0;
}


int
timer_destroy(struct taptimer_t* timer)
{
    pthread_attr_destroy(&(timer->thread_attr));
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <libevdev.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>
#include <math.h>

#define MAXEVENTS 1
#define MARGIN 5

int last_value = -1;

typedef enum {
    EDGE_NONE,
    EDGE_LEFT,
    EDGE_TOP,
    EDGE_RIGHT,
    EDGE_BOTTOM,
    edge_len
} edge_t;
static const char* edges[edge_len] = {
    [EDGE_NONE] = "None",
    [EDGE_LEFT] = "Left",
    [EDGE_TOP] = "Top",
    [EDGE_RIGHT] = "Right",
    [EDGE_BOTTOM] = "Bottom"
};

struct device_t {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    edge_t handling;
} d;

void printEvent(struct input_event * ev) {
    printf("Key: %s %s %d\n", 
            libevdev_event_type_get_name(ev->type),
            libevdev_event_code_get_name(ev->type, ev->code),
            ev->value);
}

int handleXEvent(int value) {
    if (d.handling == EDGE_TOP || d.handling == EDGE_BOTTOM)
        return 1;

    if (!d.handling) {
        if (value < d.min_x + MARGIN)
            d.handling = EDGE_LEFT;
        else if (value > d.max_x - MARGIN)
            d.handling = EDGE_RIGHT;
    }

    if (d.handling && abs(last_value - value) > 200) {
        printf("X: %d\n", value);
        last_value = value;
    }
    return 0;
}

int handleEvent(struct input_event * ev) {
    if (ev->type == EV_KEY && ev->code == BTN_TOUCH) {
        if (ev->value == 0) {
            if (!d.handling) return 0;

            printf("%s\n", edges[d.handling]);
            d.handling = EDGE_NONE;
        }
    }
    else if(ev->type == EV_ABS) {
        switch (ev->code) {
            case ABS_MT_POSITION_X:
                handleXEvent(ev->value);
                break;
            /*case ABS_MT_POSITION_Y:*/
                /*handleYEvent(ev->value);*/
                /*break;*/
            default:
                break;
        }
    }

    return 0;
}

int main(void) {
    // device
    struct libevdev *dev = NULL;
    int fd;
    int rc = 1;

    // epoll
    int epfd;
    struct epoll_event ep_ev;
    struct epoll_event ep_events[MAXEVENTS];

    // Open the input device. Hardcoded for now.
    fd = open("/dev/input/by-id/usb-Tablet_ISD-V4-event-if01", O_RDONLY|O_NONBLOCK);
    if (fd == -1)
        err(1, "Device not found");

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0)
        err(1, "Can't open evdev device");

    // get device capabilites (min max X Y)
    d.min_x = libevdev_get_abs_minimum(dev, ABS_MT_POSITION_X);
    d.max_x = libevdev_get_abs_maximum(dev, ABS_MT_POSITION_X);
    d.min_y = libevdev_get_abs_minimum(dev, ABS_MT_POSITION_Y);
    d.max_y = libevdev_get_abs_maximum(dev, ABS_MT_POSITION_Y);

    // create epoll instance
    epfd = epoll_create1(0);
    if (epfd == -1)
        err(1, "Can't create epoll instance");

    // tell epoll to listen on the device
    ep_ev.events = EPOLLIN;
    ep_ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ep_ev) == -1)
        err(1, "epoll_ctl fail");

    while (1) {
        int nfds = epoll_wait(epfd, ep_events, MAXEVENTS, -1);
        if (nfds == -1)
            err(1, "epoll_wait fail");

        do {
            struct input_event ev;
            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                handleEvent(&ev);
            }

        } while (rc == 1 || rc == 0);
    }

    return 0;
}


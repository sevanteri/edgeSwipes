
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <linux/input.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <libevdev.h>

#define MAXEVENTS 1
#define MARGIN 5
#define SENSITIVITY 20
#define EDGE_SENSITIVITY_PERCENT 0.05 // percent

#ifdef DEBUG
#define DPRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DPRINT(...) ((void) 0)
#endif

typedef enum edge_t {
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
typedef struct axis_t {
    int min;
    int max;
    edge_t zero_edge;
    edge_t full_edge;
} axis_t;
static struct device_t {
    axis_t X;
    axis_t Y;
    edge_t handling;
} d;

int last_value = -1;
int cur_value = -1;
int edge_sensitivity = -1;

static void
printEvent(struct input_event * ev)
{
    DPRINT("Key: %s %s %d\n",
            libevdev_event_type_get_name(ev->type),
            libevdev_event_code_get_name(ev->type, ev->code),
            ev->value);
}

static int
handleAxis(axis_t* axis, int value)
{
    if (d.handling &&
        (d.handling != axis->zero_edge && d.handling != axis->full_edge))
        return 1;

    if (!d.handling) {
        if (value < axis->min + MARGIN) {
            d.handling = axis->zero_edge;
            DPRINT("start: %s\n", edges[d.handling]);
        }
        else if (value > axis->max - MARGIN) {
            d.handling = axis->full_edge;
            DPRINT("start: %s\n", edges[d.handling]);
        }
    }

    if (d.handling) {
        int val = (d.handling == axis->zero_edge) ? value : (axis->max - value);
        if (abs(cur_value - val) > SENSITIVITY) {
            last_value = cur_value;
            cur_value = val;
            DPRINT("cur: %d, last: %d\n", cur_value, last_value);
        }

    }

    return 0;
}

static int
handleTouch(int value)
{
    if (value == 0) {
        if (!d.handling) return 0;

        DPRINT("end: %s\n", edges[d.handling]);
        if (cur_value > edge_sensitivity && cur_value >= last_value) {
            printf("%s\n", edges[d.handling]);
            d.handling = EDGE_NONE;
        }
        last_value = -1;
        cur_value = -1;
    }

    return 0;
}

static int
handleEvent(struct input_event * ev)
{
    if (ev->type == EV_KEY && ev->code == BTN_TOUCH) {
        handleTouch(ev->value);
    }
    else if(ev->type == EV_ABS) {
        switch (ev->code) {
        case ABS_MT_POSITION_X:
            handleAxis(&d.X, ev->value);
            break;
        case ABS_MT_POSITION_Y:
            handleAxis(&d.Y, ev->value);
            break;
        default:
            break;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    // parse arguments
    int c;
    /*FILE *config_file;*/
    char* device_path = "/dev/input/by-id/usb-Tablet_ISD-V4-event-if01";
    while ((c = getopt(argc, argv, "d:c:")) != -1) {
        switch (c) {
            case 'c':
                /*config_file = fopen(optarg, O_RDONLY);*/
                break;
            case 'd':
                device_path = optarg;
                break;
            case '?':
                if (optopt == 'c' || optopt == 'd')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
                return 1;
            default:
                exit(1);
        }
    }

    // device
    struct libevdev *dev = NULL;
    int fd;
    int rc = 1;

    // epoll
    int epfd;
    struct epoll_event ep_ev;
    struct epoll_event ep_events[MAXEVENTS];

    // Open the input device. Hardcoded for now.
    fd = open(device_path, O_RDONLY|O_NONBLOCK);
    if (fd == -1)
        err(1, "Device not found");

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0)
        err(1, "Can't open evdev device");

    DPRINT("Device name: %s\n", libevdev_get_name(dev));

    // get device capabilites (min max X Y)
    // and initialize axis'
    d.X = (struct axis_t){
        .min = libevdev_get_abs_minimum(dev, ABS_MT_POSITION_X),
        .max = libevdev_get_abs_maximum(dev, ABS_MT_POSITION_X),
        .zero_edge = EDGE_LEFT,
        .full_edge = EDGE_RIGHT
    };
    d.Y = (struct axis_t){
        .min = libevdev_get_abs_minimum(dev, ABS_MT_POSITION_Y),
        .max = libevdev_get_abs_maximum(dev, ABS_MT_POSITION_Y),
        .zero_edge = EDGE_TOP,
        .full_edge = EDGE_BOTTOM
    };

    // same sensitivity for both axis
    edge_sensitivity = MIN(
            d.X.max - d.X.min,
            d.Y.max - d.Y.min
    ) * EDGE_SENSITIVITY_PERCENT;

    DPRINT("edge sensitivity: %d\n", edge_sensitivity);

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


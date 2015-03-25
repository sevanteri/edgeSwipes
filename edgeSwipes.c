
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <linux/input.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "edgeSwipes.h"
#include "tapper.h"


/*
 *static void
 *printEvent(struct input_event * ev)
 *{
 *    DPRINT("Key: %s %s %d\n",
 *            libevdev_event_type_get_name(ev->type),
 *            libevdev_event_code_get_name(ev->type, ev->code),
 *            ev->value);
 *}
 */

static int
handleAxis(device_t *d, axis_t* axis, int value)
{
    if (d->handling &&
        (d->handling != axis->zero_edge && d->handling != axis->full_edge))
        return 1;

    if (!d->handling) {
        if (value < axis->min + MARGIN) {
            d->handling = axis->zero_edge;
            d->last_value = axis->min;
            DPRINT("start: %s\n", edges[d->handling]);
        }
        else if (value > axis->max - MARGIN) {
            d->handling = axis->full_edge;
            d->last_value = axis->max;
            DPRINT("start: %s\n", edges[d->handling]);
        }
    }

    if (d->handling) {
        int val = (d->handling == axis->zero_edge) ?
            value :
            (axis->max - value);

        if (abs(d->cur_value - val) > SENSITIVITY) {
            d->last_value = d->cur_value;
            d->cur_value = val;
            DPRINT("cur: %d, last: %d\n", d->cur_value, d->last_value);
        }

    }

    return 0;
}

static int
handleTouch(device_t *d, int value)
{
    if (value == 1) {
        clock_gettime(CLOCK_REALTIME, &(d->tapper.start));
    }
    else if (value == 0) {
        clock_gettime(CLOCK_REALTIME, &(d->tapper.end));

        int diff = d->tapper.end.tv_nsec - d->tapper.start.tv_nsec;
        int sec = d->tapper.end.tv_sec - d->tapper.start.tv_sec;
        if (sec < 1 && diff < TAPSENSITIVITY) {
            DPRINT("TAP %d / %d, %f\n", diff, sec, TAPSENSITIVITY);
            tapper_run(&(d->tapper));
        } else {
            DPRINT("NO TAP %d / %d, %f\n", diff, sec, TAPSENSITIVITY);
        }

        if (!d->handling) return 0;

        DPRINT("end: %s\n", edges[d->handling]);
        if (d->cur_value > d->edge_sensitivity &&
                d->cur_value >= d->last_value) {
            printf("%s\n", edges[d->handling]);
        }
        d->last_value = -1;
        d->cur_value = -1;
        d->handling = EDGE_NONE;
    }

    return 0;
}

static int
handleEvent(device_t *d, struct input_event * ev)
{
    if (ev->type == EV_KEY && ev->code == BTN_TOUCH) {
        handleTouch(d, ev->value);
    }
    else if(ev->type == EV_ABS) {
        switch (ev->code) {
        case ABS_MT_POSITION_X:
            handleAxis(d, &d->X, ev->value);
            break;
        case ABS_MT_POSITION_Y:
            handleAxis(d, &d->Y, ev->value);
            break;
        default:
            break;
        }
    }

    return 0;
}

static int
deviceFromPath(device_t *d, const char* path)
{
    int rc = 0;
    d->fd = open(path, O_RDONLY|O_NONBLOCK);
    if (d->fd == -1)
        err(1, "Device not found");

    rc = libevdev_new_from_fd(d->fd, &(d->dev));
    if (rc < 0)
        err(1, "Can't open evdev device");

    DPRINT("Device name: %s\n", libevdev_get_name(d->dev));

    if (!libevdev_has_event_type(d->dev, EV_ABS)) {
        err(1, "Device doesn't seem to be a mouse/touchpad/touchscree");
    }

    return 0;
}

static void*
printTapCount(void* arg) {
    struct tapper_t *tapper = (struct tapper_t*)arg;
    printf("tap_%d\n", tapper->count);
    return NULL;
}

static int
getDevice(device_t* d, char* path, const char* name, config_t* cfg)
{

    // check if path is set
    // -> choose dev with path
    if (strlen(path) > 0) {
        deviceFromPath(d, path);

        // get device capabilites (min max X Y)
        // and initialize axis'
        d->X = (struct axis_t){
            .min = libevdev_get_abs_minimum(d->dev, ABS_MT_POSITION_X),
            .max = libevdev_get_abs_maximum(d->dev, ABS_MT_POSITION_X),
        };
        d->Y = (struct axis_t){
            .min = libevdev_get_abs_minimum(d->dev, ABS_MT_POSITION_Y),
            .max = libevdev_get_abs_maximum(d->dev, ABS_MT_POSITION_Y),
        };

        goto finalize;
    }

    // check if name is set
    // -> read config and try to find it from there
    // -> check if device path in config
    // -> if yes, use it, if not, loop through evdev devices and choose
    if (name && cfg) {
        DPRINT("Searching for '%s'\n", name);
        config_setting_t *setting = config_lookup(cfg, name);
        if (setting == NULL) {
            fprintf(stderr, "Option not found\n");
            exit(1);
        }

        char *opt_dev_path = malloc(strlen(name) + 12);
        strcpy(opt_dev_path, name);
        strcat(opt_dev_path, ".devicePath");

        const char* dev_path = "";

        config_lookup_string(cfg, opt_dev_path, &dev_path);

        if (strlen(dev_path) <= 0) {
            fprintf(stderr, "Device path not set\n");
            return -1;
        }

        deviceFromPath(d, dev_path);
        free(opt_dev_path);

        int value = -1;
        char *option = malloc(strlen(name) + 7);
        int min, max;

        // Read minimum and maximum values for both axis from the config file
        strcpy(option, name);
        strcat(option, ".min_x");
        config_lookup_int(cfg, option, &value);
        min = value;
        value = -1;

        strcpy(option, name);
        strcat(option, ".max_x");
        config_lookup_int(cfg, option, &value);
        max = value;
        value = -1;

        d->X = (struct axis_t){
            .min = (min != -1) ?
                min :
                libevdev_get_abs_minimum(d->dev, ABS_MT_POSITION_X),

            .max = (max != -1) ?
                max :
                libevdev_get_abs_maximum(d->dev, ABS_MT_POSITION_X),
        };

        strcpy(option, name);
        strcat(option, ".min_y");
        config_lookup_int(cfg, option, &value);
        min = value;
        value = -1;

        strcpy(option, name);
        strcat(option, ".max_y");
        config_lookup_int(cfg, option, &value);
        max = value;
        value = -1;

        d->Y = (struct axis_t){
            .min = (min != -1) ?
                min :
                libevdev_get_abs_minimum(d->dev, ABS_MT_POSITION_Y),

            .max = (max != -1) ?
                max :
                libevdev_get_abs_maximum(d->dev, ABS_MT_POSITION_Y),
        };

        DPRINT("X.min: %d\nX.max: %d\nY.min: %d\nY.max: %d\n",
                d->X.min, d->X.max, d->Y.min, d->Y.max
                );

        free(option);

        goto finalize;
    }



finalize:
    d->X.zero_edge = EDGE_LEFT;
    d->X.full_edge = EDGE_RIGHT;
    d->Y.zero_edge = EDGE_TOP;
    d->Y.full_edge = EDGE_BOTTOM;

    d->handling = EDGE_NONE;

    d->cur_value = -1;
    d->last_value = -1;

    // same sensitivity for both axis
    int xdiff = d->X.max - d->X.min;
    int ydiff = d->Y.max - d->Y.min;
    d->edge_sensitivity = MIN(xdiff, ydiff) * EDGE_SENSITIVITY_PERCENT;

    DPRINT("edge sensitivity: %d\n", d->edge_sensitivity);

    tapper_init(&(d->tapper), TAPSENSITIVITY, printTapCount);

    return 0;
}

int main(int argc, char **argv)
{
    // parse arguments
    int c;
    int config_read = 0;

    config_t cfg;
    config_init(&cfg);

    /*char* device_path = "/dev/input/by-id/usb-Tablet_ISD-V4-event-if01";*/
    char* device_path = "";
    char* search_name = "default";

    while ((c = getopt(argc, argv, "n:d:c:")) != -1) {
        switch (c) {
            case 'c':
                if (!config_read_file(&cfg, optarg)) {
                    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                            config_error_line(&cfg), config_error_text(&cfg));
                    config_destroy(&cfg);
                    exit(1);
                }
                DPRINT("Reading config file %s\n", optarg);
                config_read = 1;
                break;
            case 'd':
                device_path = optarg;
                break;
            case 'n':
                search_name = optarg;
                break;
            default:
                return 1;
        }
    }

    if (!config_read) {
        if (!config_read_file(&cfg, CONFIG_FILE)) {
            fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                    config_error_line(&cfg), config_error_text(&cfg));
            config_destroy(&cfg);
            exit(1);
        }
        DPRINT("Reading config file %s\n", CONFIG_FILE);
    }

    // device
    device_t d;
    if (getDevice(&d, device_path, search_name, &cfg) < 0) {
        err(1, "Failed to initialize device\n");
        return 1;
    }

    // epoll
    int epfd;
    struct epoll_event ep_ev = {0};
    struct epoll_event ep_events[MAXEVENTS];

    // create epoll instance
    epfd = epoll_create1(0);
    if (epfd == -1)
        err(1, "Can't create epoll instance");

    // tell epoll to listen on the device
    ep_ev.events = EPOLLIN;
    ep_ev.data.fd = d.fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, d.fd, &ep_ev) == -1)
        err(1, "epoll_ctl fail");

    int rc = 1;
    while (1) {
        int nfds = epoll_wait(epfd, ep_events, MAXEVENTS, -1);
        if (nfds == -1)
            err(1, "epoll_wait fail");

        do {
            struct input_event ev;
            rc = libevdev_next_event(d.dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                handleEvent(&d, &ev);
            }

        } while (rc == 1 || rc == 0);
    }

    return 0;
}


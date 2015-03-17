
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <libevdev.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>

#define MAXEVENTS 1

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

    printf("Input name: %s", libevdev_get_name(dev));

    // create epoll instance
    epfd = epoll_create1(0);
    if (epfd == -1)
        err(1, "Can't create epoll instance");

    // tell epoll to listen on the device
    ep_ev.events = EPOLLIN;
    ep_ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ep_ev) == -1)
        err(1, "epoll_ctl fail");

    struct input_event ev;
    int nfds, i;

    while (1) {
        nfds = epoll_wait(epfd, ep_events, MAXEVENTS, -1);
        if (nfds == -1)
            err(1, "epoll_wait fail");

        do {
            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_X)
                    printf("Event: %s %s %d\n",
                            libevdev_event_type_get_name(ev.type),
                            libevdev_event_code_get_name(ev.type, ev.code),
                            ev.value
                          );
            }

        } while (rc == 1 || rc == 0 || rc == -EAGAIN);
    }

    return 0;
}

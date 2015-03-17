
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <libevdev.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>

int main(void) {
    struct libevdev *dev = NULL;
    int fd,
        rc = 1,
        epfd;

    fd = open("/dev/input/by-id/usb-Tablet_ISD-V4-event-if01", O_RDONLY|O_NONBLOCK);
    if (fd < 0)
        err(1, "Input not found");

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0)
        err(1, "Lol");

    printf("Input name: %s", libevdev_get_name(dev));

    do {
        struct input_event ev;
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

    return 0;
}

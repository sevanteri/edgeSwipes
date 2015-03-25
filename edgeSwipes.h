#include <libevdev.h>
#include <libconfig.h>
#include "tapper.h"

#define MAXEVENTS 1
#define MARGIN 5
#define SENSITIVITY 20
#define TAPSENSITIVITY 2E8 // 100 milli seconds
#define EDGE_SENSITIVITY_PERCENT 0.05 // percent
#define CONFIG_FILE "config.cfg"

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

typedef struct device_t {
    axis_t X;
    axis_t Y;
    edge_t handling;

    int fd;
    struct libevdev *dev;

    int last_value;
    int cur_value;
    int edge_sensitivity;

    tapper_t tapper;
} device_t;


static int
handleAxis(device_t *d, axis_t* axis, int value);
static int
handleTouch(device_t *d, int value);
static int
handleEvent(device_t *d, struct input_event * ev);
static int
getDevice(device_t* d, char* path, const char* name, config_t* cfg);
static int
deviceFromPath(device_t *d, const char* path);

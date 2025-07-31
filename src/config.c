#include "config.h"

config_t g_config = {
    .verbose_logging = false,
    .window_scale = 10,
    .cycle_delay = 1,
    .bg_color = {
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 255,
    },
    .fg_color = {
        .r = 255,
        .g = 255,
        .b = 255,
        .a = 255,
    },
};

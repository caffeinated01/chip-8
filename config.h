#pragma once
#include <stdbool.h>

typedef struct config
{
  bool verbose_logging;
  int window_scale;
} config_t;

extern config_t g_config;

#pragma once
#include <stdbool.h>

typedef struct config {
  bool verbose_logging;
} config_t;

extern config_t g_config;

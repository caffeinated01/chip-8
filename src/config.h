#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct Color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} color_t;

typedef struct config
{
  bool verbose_logging;
  int window_scale;
  int cycle_delay;
  color_t bg_color;
  color_t fg_color;
} config_t;

extern config_t g_config;

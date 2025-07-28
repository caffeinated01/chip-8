#pragma once
#include <stdio.h>
#include "config.h"

#define LOG_RESET "\x1b[0m"
#define LOG_RED "\x1b[31m"
#define LOG_GREEN "\x1b[32m"
#define LOG_BLUE "\x1b[34m"

/**
 * @brief informational log message, only executes if verbose logging is enabled
 * @param fmt the format string
 * @param ... arguments following the format string
 */
#define LOG_INFO(fmt, ...)                                                  \
  do                                                                        \
  {                                                                         \
    if (g_config.verbose_logging)                                           \
      fprintf(stdout, LOG_BLUE "[INFO] " LOG_RESET fmt "\n", ##__VA_ARGS__); \
  } while (0)

/**
 * @brief success log message, only executes if verbose logging is enabled
 *
 * @param fmt the format string.
 * @param ... arguments following the format string
 */
#define LOG_OK(fmt, ...)                                                   \
  do                                                                       \
  {                                                                        \
    if (g_config.verbose_logging)                                          \
      fprintf(stdout, LOG_GREEN "[OK] " LOG_RESET fmt "\n", ##__VA_ARGS__); \
  } while (0)

/**
 * @brief error log message, executes regardless of whether verbose logging is enabled
 *
 * @param fmt the format string
 * @param ... arguments following the format string
 */
#define LOG_ERROR(fmt, ...) fprintf(stderr, LOG_RED "[ERROR] " LOG_RESET fmt "\n", ##__VA_ARGS__)

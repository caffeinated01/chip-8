#pragma once
#include <stdio.h>

#define LOG_RESET "\x1b[0m"
#define LOG_RED "\x1b[31m"
#define LOG_GREEN "\x1b[32m"
#define LOG_BLUE "\x1b[34m"

// what ##__VA_ARGS__ does: https://en.cppreference.com/w/c/variadic/va_arg

/**
 * @brief logger for success msg
 */
#define LOG_OK(fmt, ...) fprintf(stdout, LOG_GREEN "[OK] " LOG_RESET fmt "\n", ##__VA_ARGS__)

/**
 * @brief logger for error msg
 */
#define LOG_ERROR(fmt, ...) fprintf(stderr, LOG_RED "[ERROR] " LOG_RESET fmt "\n", ##__VA_ARGS__)

/**
 * @brief logger for displaying info
 */
#define LOG_INFO(fmt, ...) fprintf(stdout, LOG_BLUE "[INFO] " LOG_RESET fmt "\n", ##__VA_ARGS__)

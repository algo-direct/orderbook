#pragma once

#include <iostream>
#include <thread>

void printTimestamp(bool withNewLine = false);

#define LOG_LINE(level, ...)                                        \
  do {                                                              \
    printTimestamp();                                               \
    std::cout << " " << level << __FILE__ << ":" << __LINE__ << " " \
              << std::this_thread::get_id() << " " << __VA_ARGS__   \
              << std::endl;                                         \
  } while (false)

#define ERROR_LEVEL 1
#define WARN_LEVEL 2
#define INFO_LEVEL 3
#define DEBUG_LEVEL 4
#define TRACE_LEVEL 5

#ifndef LOG_LEVEL
#define LOG_LEVEL INFO_LEVEL
#endif

#define LOG_ERROR(...) LOG_LINE(" ERROR ", __VA_ARGS__)

#if LOG_LEVEL >= WARN_LEVEL
#define LOG_WARN(...) LOG_LINE(" WARN ", __VA_ARGS__)
#else
#define LOG_WARN(...)
#endif

#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(...) LOG_LINE(" INFO ", __VA_ARGS__)
#else
#define LOG_INFO(...)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(...) LOG_LINE(" DEBUG ", __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#if LOG_LEVEL >= TRACE_LEVEL
#define LOG_TRACE(...) LOG_LINE(" TRACE ", __VA_ARGS__)
#else
#define LOG_TRACE(...)
#endif

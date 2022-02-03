#pragma once

#ifdef LOGGER_ENABLE_ALL
#define LOGGER_ENABLE_CRITICAL
#define LOGGER_ENABLE_ERROR
#define LOGGER_ENABLE_WARN
#define LOGGER_ENABLE_INFO
#define LOGGER_ENABLE_DEBUG
#define LOGGER_ENABLE_TRACE
#endif

#if defined(LOGGER_ENABLE_CRITICAL) || defined(LOGGER_ENABLE_ERROR) || defined(LOGGER_ENABLE_WARN) \
    || defined(LOGGER_ENABLE_INFO) || defined(LOGGER_ENABLE_DEBUG) || defined(LOGGER_ENABLE_TRACE)
#include <stdio.h>
#endif

#ifdef LOGGER_ENABLE_CRITICAL
#define LOG_CRITICAL(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "CRT", __FUNCTION__, ##args)
#else
#define LOG_CRITICAL(message, args...) (void)0
#endif

#ifdef LOGGER_ENABLE_ERROR
#define LOG_ERROR(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "ERR", __FUNCTION__, ##args)
#else
#define LOG_ERROR(message, args...) (void)0
#endif

#ifdef LOGGER_ENABLE_WARN
#define LOG_WARN(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "WRN", __FUNCTION__, ##args)
#else
#define LOG_WARN(message, args...) (void)0
#endif

#ifdef LOGGER_ENABLE_INFO
#define LOG_INFO(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "IFO", __FUNCTION__, ##args)
#else
#define LOG_INFO(message, args...) (void)0
#endif

#ifdef LOGGER_ENABLE_DEBUG
#define LOG_DEBUG(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "DBG", __FUNCTION__, ##args)
#else
#define LOG_DEBUG(message, args...) (void)0
#endif

#ifdef LOGGER_ENABLE_TRACE
#define LOG_TRACE(message, args...) fprintf(stderr, "%3s@%s: " message "\n", "TRC", __FUNCTION__, ##args)
#else
#define LOG_TRACE(message, args...) (void)0
#endif
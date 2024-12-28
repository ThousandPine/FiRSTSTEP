#pragma once

#include "varg.h"

#define BMB __asm__ volatile("xchgw %bx, %bx");

#define assert(exp)                                 \
    if (!(exp))                                     \
    {                                               \
        assertion_failed(#exp, __FILE__, __BASE_FILE__, __LINE__); \
    }                                               \
    else                                            \
    {                                               \
    }

#ifdef DEBUG
#define DEBUGK(fmt, ...) \
    debug_print(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define DEBUGK(fmt, ...) // do nothing
#endif

int printk(const char *fmt, ...);
int vprintk(const char *fmt, va_list args);
void assertion_failed(const char *exp, const char *file, const char *base, int line);
void panic(const char *fmt, ...);
void debug_print(const char *file, int line, const char *fmt, ...);

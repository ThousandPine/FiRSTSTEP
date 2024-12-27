#pragma once

#include "varg.h"

#define assert(exp)                                 \
    if (!(exp))                                     \
    {                                               \
        assertion_failed(#exp, __FILE__, __BASE_FILE__, __LINE__); \
    }                                               \
    else                                            \
    {                                               \
    }

int printk(const char *fmt, ...);
int vprintk(const char *fmt, va_list args);
void assertion_failed(const char *exp, const char *file, const char *base, int line);
void panic(const char *fmt, ...);

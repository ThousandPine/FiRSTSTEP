#include "varg.h"
#include "unistd.h"
#include "stdio.h"
#include "kernel/kernel.h"

int printf(const char *fmt, ...)
{
    static char buf[1024];

    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);

    return write(STDOUT, buf, len);
}
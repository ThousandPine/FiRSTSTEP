#include "tty.h"
#include "stdio.h"

int vprintk(const char *fmt, va_list args)
{
    static char buf[1024];

    int n = 0;
    n = vsprintf(buf, fmt, args);
    tty_write(buf, n);
    return n;
}

int printk(const char *fmt, ...)
{
    int n = 0;
    va_list args = va_start(fmt);
    n = vprintk(fmt, args);
    va_end(args);
    return n;
}


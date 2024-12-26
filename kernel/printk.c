#include "tty.h"
#include "stdio.h"

static char buf[1024];

int printk(const char *fmt, ...)
{
    int n = 0;
    va_list args = va_start(fmt);
    n = vsprintf(buf, fmt, args);
    va_end(args);
    tty_write(buf, n);
    return n;
}

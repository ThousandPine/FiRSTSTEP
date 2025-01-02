#include "kernel/kernel.h"

static void spin(void)
{
    while (1)
        ;
}

/**
 * @warning 不能在终端初始化前使用
 * @warning 不能在 print 相关函数中调用，否则会引发递归
 */
void assertion_failed(const char *exp, const char *file, const char *base, int line)
{
    printk(
        "\n################ ERROR ################\n"
        "assert(%s) failed\n"
        "file: %s:%d\n"
        "base: %s"
        "\n#######################################\n",
        exp, file, line, base);
    spin();
}

/**
 * @warning 不能在终端初始化前使用
 * @warning 不能在 print 相关函数中调用，否则会引发递归
 */
void panic(const char *fmt, ...)
{
    printk("\n################ PANIC ################\n");

    va_list args = va_start(fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("\n#######################################\n");

    spin();
}

/**
 * @warning 不能在终端初始化前使用
 * @warning 不能在 print 相关函数中调用，否则会引发递归
 */
void debug_print(const char *file, int line, const char *fmt, ...)
{
    printk("[%s] [%d] ", file, line);

    va_list args = va_start(fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("\n");
}

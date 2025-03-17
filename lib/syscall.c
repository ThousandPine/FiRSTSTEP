#include "types.h"
#include "varg.h"

int syscall(int syscall_no, ...)
{
    va_list args = va_start(syscall_no);

    int32_t arg1 = va_arg(args, int32_t);
    int32_t arg2 = va_arg(args, int32_t);
    int32_t arg3 = va_arg(args, int32_t);
    
    va_end(args);
    
    // 进行系统调用
    int ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (syscall_no), "b" (arg1), "c" (arg2), "d" (arg3)
        : "memory"
    );
    return ret;
}
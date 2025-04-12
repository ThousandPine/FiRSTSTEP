#include "types.h"
#include "varg.h"
#include "kernel/syscall.h"

int syscall(int syscall_no, ...)
{
    va_list args;
    va_start(args, syscall_no);

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

int write(int fd, const void *buf, int count)
{
    return syscall(SYS_NR_WRITE, fd, buf, count);
}

int fork(void)
{
    return syscall(SYS_NR_FORK);
}
pid_t getpid(void)
{
    return syscall(SYS_NR_GETPID);
}
void exit(int status)
{
    syscall(SYS_NR_EXIT, status);
}
pid_t wait(int *status)
{
    return syscall(SYS_NR_WAIT, status);
}
pid_t waitpid(pid_t pid, int *status, int options)
{
    return syscall(SYS_NR_WAITPID, pid, status, options);
}

/**
 * @param path 可执行文件绝对路径
 * @param arg0 参数列表特地声明 arg0 是因为要保证至少有一个参数，即末尾的 NULL
 * @param ... 可变参数列表，最后一个参数必须是 NULL
 */
int execl(const char *path, const char *arg0, ...)
{
    va_list args;
    // 以 path 为基准，这样 va_list 就可以从 arg0 开始
    va_start(args, path);

    int ret = syscall(SYS_NR_EXECL, path, args);
    
    va_end(args);
    
    return ret;
}
#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    printf("init process %d\n", getpid());
    // 两次 fork 最终会有 4 个进程
    fork();
    fork();
    // 每个进程都循环打印进程 ID
    while (1)
    {
        printf("running process %d\n", getpid());
        // 延时，控制输出频率
        for (int i = 2e6; i >0; i--)
        {
            // do nothing
        }
    }
}
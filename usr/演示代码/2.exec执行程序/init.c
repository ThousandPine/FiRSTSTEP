#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    printf("init process %d\n", getpid());

    if (execl("/bin/hello", NULL) == -1)
    {
        printf("exec failed\n");
        return -1;
    }
    // 不会执行到此处

    return 1234;
}
#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

void fork_func(int i)
{
    if (i <= 0)
    {
        return;
    }
    int pid = fork();
    if (pid == -1)
    {
        printf("fork failed\n");
    }
    else if (pid == 0)
    {
        printf("I am child %d\n", getpid());
        fork_func(i - 1);
    }
    else
    {
        printf("I am parent %d, child pid = %d\n", getpid(), pid);
    }
}

int main(void)
{
    BMB;
    BMB;
    int n = printf("Hello, world!\n");
    fork_func(5);
    while(1)
    {
    }
    return 0;
}
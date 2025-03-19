#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    BMB;
    BMB;
    int n = printf("Hello, world!\n");
    int pid = fork();
    if (pid == -1)
    {
        printf("fork failed\n");
    }
    else if (pid == 0)
    {
        printf("I am child\n");
    }
    else
    {
        printf("I am parent, child pid = %d\n", pid);
    }
    while(1)
    {
    }
    return 0;
}
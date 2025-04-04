#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    // BMB;
    // BMB;
    int n = printf("Hello, world!\n");
    
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("fork failed\n");
    }
    else if (pid == 0)
    {
        printf("I am child %d\n", getpid());
        // for (int i = 10000; i-- > 0;)
        // {
        // }
        exit(233);
    }
    else
    {
        printf("I am parent %d, child pid = %d\n", getpid(), pid);
        int status = 0;
        pid = wait(&status);
        printf("child %d exited with status %d\n", pid, status);
    }

    while(1)
    {
        // printf("task %d running", getpid());
    }
    return 0;
}
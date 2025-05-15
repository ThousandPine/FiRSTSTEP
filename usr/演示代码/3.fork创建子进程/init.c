#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    printf("init process %d\n", getpid());
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("fork failed\n");
        return -1;
    }
    // 子进程
    else if (pid == 0)
    {
        printf("child process %d\n", getpid());
        
        // 延时
        for (int i = 3e7; i >0; i--)
        {
            // do nothing
        }
        
        printf("child process %d end\n", getpid());
        return 123;
    }
    // 父进程
    else
    {
        printf("parent process %d\n", getpid());
        int status;
        // 等待子进程结束
        pid_t wpid = wait(&status);
        if (wpid != -1)
        {
            printf("child process %d exited with status %d\n", wpid, status);
        }
        return 0;
    }
}
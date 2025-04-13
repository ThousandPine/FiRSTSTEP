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
    else if (pid == 0)
    {
        // child process
        printf("child process %d\n", getpid());
        if (execl("/bin/hello", NULL) == -1)
        {
            printf("execve failed\n");
            return -1;
        }
    }
    else
    {
        // parent process
        printf("parent process %d\n", getpid());
        int status;
        while (1)
        {
            pid_t wpid = wait(&status);
            if (wpid != -1)
            {
                printf("child process %d exited with status %d\n", wpid, status);
            }
        }
    }

    return 0;
}
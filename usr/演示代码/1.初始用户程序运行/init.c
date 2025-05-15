#include "kernel/kernel.h"
#include "stdio.h"
#include "unistd.h"

int main(void)
{
    printf("init process %d\n", getpid());
    return 1234;
}
#include "kernel/kernel.h"
#include "syscall.h"

int main(void)
{
    BMB;
    BMB;
    syscall(0, 1, 2, 3);
    while(1)
    {
    }
    return 0;
}
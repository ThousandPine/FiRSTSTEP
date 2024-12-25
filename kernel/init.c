/* 内核测试代码 */
#include "tty.h"

void puts(char *s);

int main(void)
{
    tty_init();

    puts("In Kernel Init!!!");

    while(1);
}

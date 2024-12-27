/* 内核测试代码 */
#include "kernel/tty.h"
#include "kernel/kernel.h"

int main(void)
{
    tty_init();

    printk("Take the FiRSTSTEP!");

    while(1);
}

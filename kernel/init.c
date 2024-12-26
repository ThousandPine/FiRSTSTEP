/* 内核测试代码 */
#include "tty.h"
#include "kernel.h"

int main(void)
{
    tty_init();

    printk("Take the FiRSTSTEP!");

    while(1);
}

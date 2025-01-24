#include "kernel/kernel.h"

void tty_init(void);
void mem_init(void);
void fs_init(void);
void idt_init(void);
void pic_init(void);

int init(void)
{
    tty_init();

    mem_init();

    idt_init();
    pic_init();

    fs_init();

    // 内核不能 return，并且返回地址已经在初始化栈时丢失了
    panic("The kernel runs to the end");
}

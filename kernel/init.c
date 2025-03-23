#include "kernel/kernel.h"
#include "kernel/x86.h"

void tty_init(void);
void mem_init(void);
void fs_init(void);
void idt_init(void);
void pic_init(void);
void syscall_init(void);
void task_init(void);

void init(void)
{
    tty_init();

    idt_init();
    pic_init();
    sti();

    mem_init();
    syscall_init();

    fs_init();

    task_init();

    // 内核不能 return，并且返回地址已经在初始化栈时丢失了
    panic("The kernel runs to the end");
}

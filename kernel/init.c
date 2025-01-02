/* 内核测试代码 */
#include "kernel/tty.h"
#include "kernel/kernel.h"

void mem_init(void);

static char stack[2048]; // 内核栈空间

/**
 * 初始化内核栈
 *
 * 修改栈顶寄存器 esp 使其指向变量 stack 的空间
 * 内部包括 ret 指令，所以不能对该函数使用 inline
 */
static __attribute__((noinline)) void stack_init(void *stack_top)
{
    // 设置新的栈会导致返回地址丢失，所以要提前记录
    void *addr = __builtin_return_address(0);
    asm volatile(
        "mov %0, %%esp\n"           // 将新的栈指针设置到 esp 寄存器
        "push %1\n"                 // 将函数返回地址压栈
        "ret\n"                     // 需要提前返回，否则后续会执行 leave 恢复栈帧
        :                           // 输出部分
        : "r"(stack_top), "r"(addr) // 输入部分
    );
}

int main(void)
{
    /**
     * 局部变量存储在栈空间
     * 所以初始化栈之前不得定义任何局部变量
     */
    stack_init(stack + sizeof(stack));

    tty_init();

    mem_init();

    // 内核不能 return，并且返回地址已经在初始化栈时丢失了
    while (1)
        ;
}

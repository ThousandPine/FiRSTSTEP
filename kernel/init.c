/* 内核测试代码 */
#include "kernel/tty.h"
#include "kernel/kernel.h"

void mem_init(void);

static char stack[2048]; // 内核栈空间

int main(void)
{
    /**
     * 初始化内核栈
     * 修改栈顶寄存器 esp 使其指向变量 stack 的空间
     * 
     * 局部变量存储在栈空间
     * 所以初始化栈之前不得定义任何局部变量
     */
    asm volatile(
        "mov %0, %%esp\n"            // 将新的栈指针设置到 esp 寄存器
        :                            // 输出部分
        : "r"(stack + sizeof(stack)) // 输入部分
    );

    tty_init();

    mem_init();

    // 内核不能 return，并且返回地址已经在初始化栈时丢失了
    while (1)
        ;
}

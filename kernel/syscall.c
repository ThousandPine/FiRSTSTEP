#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/kernel.h"

static void* syscall_table[NR_SYSCALL];

static int sys_default(void)
{
    panic("sys_default: Unimplemented system calls");
}

static int sys_test(void)
{
    printk("test syscall!\n");
    return 2333;
}

void syscall_handler(uint32_t syscall_no, uint32_t arg1, uint32_t arg2, uint32_t arg3, interrupt_frame *frame)
{
    if (syscall_no >= NR_SYSCALL)
    {
        panic("syscall_no %d out of boundary %d", syscall_no, NR_SYSCALL);
    }

    // 调用对应系统调用函数，返回值保存在 eax 寄存器
    frame->eax = ((int(*)(uint32_t, uint32_t, uint32_t))syscall_table[syscall_no])(arg1, arg2, arg3);
}

void syscall_init(void)
{
    for (uint32_t i = 0; i < NR_SYSCALL; i++)
    {
        syscall_table[i] = sys_default;
    }
    syscall_table[SYS_NR_TEST] = sys_test;
}
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/kernel.h"
#include "kernel/tty.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "stdio.h"

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

static int sys_write(uint32_t fd, const void *buf, uint32_t count)
{
    if (fd != STDOUT)
    {
        return -1;
    }

    tty_write(buf, count);
    return count;
}

static int sys_fork(void)
{
    task_struct *new_task = fork_task(running_task());
    if (new_task == NULL)
    {
        return -1;
    }

    // 新进程的 fork() 返回值是 0
    new_task->interrupt_frame->eax = 0;

    // 将新进程加入调度队列
    switch_task_state(new_task, TASK_READY);

    return new_task->pid;
}

static pid_t sys_getpid(void)
{
    return running_task()->pid;
}

static void sys_exit(int exit_code)
{
    task_struct *task = running_task();
    if (task == NULL)
    {
        panic("running_task is NULL, exit failed");
        return;
    }

    DEBUGK("sys_exit: task %d exit_code %d", task->pid, exit_code);

    // 设置为僵尸进程
    switch_task_state(task, TASK_ZOMBIE);
    // 回收相关资源
    task_exit(task, exit_code);
    
    // 切换到下一个任务
    // 不需要传入 frame，因为该任务不会重新执行
    schedule_handler(NULL);

    // 防止返回
    panic("sys_exit: no task to switch");
}

pid_t sys_wait(int *status)
{
    task_struct *task = running_task();
    if (task == NULL)
    {
        panic("running_task is NULL, wait failed");
        return -1;
    }

    // 如果没有子进程，则返回 -1
    task_struct *child = task->child;
    if (child == NULL)
    {
        return -1;
    }

    // 等待子进程结束
    while (child->state != TASK_ZOMBIE)
    {
        // 调度其他任务
        schedule();
    }

    // 记录返回值
    pid_t pid = child->pid;
    *status = child->exit_code;
    
    // 从父进程的子进程链表中删除
    task->child = child->sibling;

    // 回收子进程的 PCB
    switch_task_state(child, TASK_DEAD);
    
    return pid;
}

static pid_t sys_waitpid(pid_t pid, int *status, int options)
{
    // TODO: 实现 waitpid
}

void syscall_handler(uint32_t syscall_no, uint32_t arg1, uint32_t arg2, uint32_t arg3, interrupt_frame *frame)
{
    /**
     * 关于段寄存器与内存访问的说明：
     * 
     * 内核态下不强制要求 DS/ES 等数据段寄存器指向内核数据段的原因：
     * 1. 内核与用户数据段均采用平坦模式，实际物理地址由分页机制管理，段选择子不影响线性地址计算。
     * 2. 数据段寄存器仅需满足 CPL <= DPL，内核态（CPL=0）可合法加载 DPL=3 的用户数据段，
     *    不会触发 #GP 异常。
     * 3. 页表项的 U/S 位决定访问权限（U/S=0 仅 CPL<3 可访问），
     *    只要 CS 为内核代码段（CPL=0），即可访问所有 U/S=0 的页面，
     *    与当前数据段选择子无关。
     * 
     * 比较特殊的是 SS 寄存器：
     *    SS 必须满足 CPL == SS.DPL，因此内核态必须使用 DPL=0 的数据段，
     *    这也是 TSS 中 SS0 必须为内核数据段选择子的根本原因
     */

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
    syscall_table[SYS_NR_WRITE] = sys_write;
    syscall_table[SYS_NR_FORK] = sys_fork;
    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_EXIT] = sys_exit;
    syscall_table[SYS_NR_WAIT] = sys_wait;
    syscall_table[SYS_NR_WAITPID] = sys_waitpid;
}
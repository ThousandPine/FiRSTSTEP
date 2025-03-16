#include "kernel/task.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/elf.h"
#include "kernel/pic.h"
#include "kernel/x86.h"
#include "kernel/scheduler.h"
#include "string.h"

static uint8_t is_used[NR_TASKS] = {0};
static task_union task_unions[NR_TASKS] __attribute__((aligned(PAGE_SIZE))) = {0};

// FIXME: 若多个任务同时调用此函数，可能会获取到同一个 task_union ，实现上需要加锁或关闭中断
static task_union* get_free_task_union(void)
{
    for (int i = 0; i < NR_TASKS; i++)
    {
        if (is_used[i] == 0)
        {
            is_used[i] = 1;
            return &task_unions[i];
        }
    }
    DEBUGK("no free task union");
    return NULL;
}

static task_struct* create_task(uint32_t entry, uint32_t code_selector, uint32_t data_selector)
{
    static int _pid = 0;

    task_union* task_union = get_free_task_union();
    if (task_union == NULL)
    {
        DEBUGK("create task failed");
        return NULL;
    }

    // 设置独特的 pid
    task_union->task.pid = _pid++;
    // 初始化 TSS ，设置内核态栈
    task_union->task.tss.ss0 = KER_DATA_SELECTOR;
    task_union->task.tss.esp0 = (uint32_t)&task_union->kernel_stack[PAGE_SIZE];
    // 初始化进程页目录
    task_union->task.page_dir = create_user_page_dir();
    // 初始化进程用户态栈
    uint32_t stack_page = pmu_alloc();
    task_union->task.usr_stack_top = PAGE_SIZE + map_physical_page(task_union->task.page_dir, stack_page, 1, 1);
    // 初始化中断栈帧，模拟中断调用
    // NOTE: 不能将中断栈帧指向 kernel_stack 的开始处，
    //       因为这是一个 union，修改 kernel_stack 前面的部分会覆盖 task 结构体
    task_union->task.interrupt_frame = (interrupt_frame *)(task_union->kernel_stack + PAGE_SIZE - sizeof(interrupt_frame));
    memset(task_union->task.interrupt_frame, 0, sizeof(interrupt_frame));
    task_union->task.interrupt_frame->ss = data_selector;
    task_union->task.interrupt_frame->esp = task_union->task.usr_stack_top;
    task_union->task.interrupt_frame->eflags = get_eflags();
    task_union->task.interrupt_frame->cs = code_selector;
    task_union->task.interrupt_frame->eip = entry;
    task_union->task.interrupt_frame->es = data_selector;
    task_union->task.interrupt_frame->ds = data_selector;
    task_union->task.interrupt_frame->fs = data_selector;
    task_union->task.interrupt_frame->gs = data_selector;

    return &task_union->task;
}

task_struct* create_task_from_elf(const char *file_path)
{
    task_struct* task = create_task(0, USER_CODE_SELECTOR, USER_DATA_SELECTOR);
    if (task == NULL)
    {
        return NULL;
    }
    // 加载 ELF 文件
    uint32_t entry = elf_loader(task->page_dir, file_path);
    if (entry == 0)
    {
        DEBUGK("load ELF file failed");
        return NULL;
    }
    // 设置用户程序入口
    task->interrupt_frame->eip = entry;

    return task;
}

void switch_to_task(task_struct *task)
{
    DEBUGK("switch to task %d", task->pid);

    // 切换 TSS
    set_tss(&task->tss);
    // 切换页目录
    switch_to_user_page(task->page_dir);
    // 切换上下文
    asm volatile(
        "mov %0, %%esp\n"
        "pop %%gs\n"
        "pop %%fs\n"
        "pop %%es\n"
        "pop %%ds\n"
        "popa\n"
        "iret"
        :
        : "r"(task->interrupt_frame)
        : "memory");
}

void task_init(void)
{
    // 创建首个任务
    task_struct *first_task = create_task_from_elf("/hello.bin");
    
    // 初始化调度器
    scheduler_init(first_task);
}

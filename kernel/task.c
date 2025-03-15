#include "kernel/task.h"
#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/elf.h"
#include "kernel/timer.h"
#include "kernel/pic.h"
#include "kernel/x86.h"
#include "string.h"

/**
 * 中断栈帧
 * 
 * 包括两个部分：中断压栈的寄存器，我们主动压栈的寄存器
 * 
 * NOTE: 根据栈向低地址增长的特性，定义变量时要让后压栈的在前，先压栈的在后
 * NOTE: 虽然部分寄存器是 2 字节位宽，但 32 位模式下压栈的单位是 4 字节，所以统一使用 4 字节大小类型
 */
typedef struct interrupt_frame
{
    // 主动压栈部分
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_useless, ebx, edx, ecx, eax; // PUSHA
    // 中断压栈部分（忽略错误码）
    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed)) interrupt_frame;

enum task_state
{
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD,
};

typedef struct task_struct
{
    tss_struct tss;
    uint32_t pid;
    uint32_t state;
    uint32_t usr_stack_top;
    uint32_t entry;
    interrupt_frame *interrupt_frame;
    page_dir_entry *page_dir;
    struct task_struct* prev;
    struct task_struct* next;
} task_struct;

typedef union task_union
{
    task_struct task;
    uint8_t kernel_stack[PAGE_SIZE];
} task_union;

static task_union task_unions[NR_TASKS] __attribute__((aligned(PAGE_SIZE))) = {0};
static task_struct* task_list = NULL;
static task_struct* current_task = NULL;

// FIXME: 若多个任务同时调用此函数，可能会获取到同一个 task_union ，实现上需要加锁或关闭中断
static task_union* get_free_task_union(void)
{
    for (int i = 0; i < NR_TASKS; i++)
    {
        if (task_unions[i].task.state == TASK_DEAD)
        {
            return &task_unions[i];
        }
    }
    DEBUGK("no free task union");
    return NULL;
}

// FIXME: 如果执行时被中断打断，可能会导致未初始化完毕的任务被调度
static task_struct* create_task(uint32_t entry, uint32_t code_selector, uint32_t data_selector)
{
    static int _pid = 0;

    task_union* task_union = get_free_task_union();
    if (task_union == NULL)
    {
        DEBUGK("create task failed");
        return NULL;
    }

    // 以插入头结点的方式加入任务链表
    task_union->task.prev = task_union->task.next = NULL;
    if (task_list != NULL)
    {
        task_union->task.next = task_list;
        task_list->prev = &task_union->task;
    }
    task_list = &task_union->task;

    // 设置独特的 pid
    task_union->task.pid = _pid++;
    // 初始化 TSS ，设置内核态栈
    task_union->task.tss.ss0 = KER_DATA_SELECTOR;
    task_union->task.tss.esp0 = (uint32_t)&task_union->kernel_stack[PAGE_SIZE];
    // 初始化进程页目录
    task_union->task.page_dir = create_user_page_dir();
    // 加载 ELF 文件
    task_union->task.entry = entry;
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
    task_union->task.interrupt_frame->eip = (uint32_t)task_union->task.entry;
    task_union->task.interrupt_frame->es = data_selector;
    task_union->task.interrupt_frame->ds = data_selector;
    task_union->task.interrupt_frame->fs = data_selector;
    task_union->task.interrupt_frame->gs = data_selector;

    return &task_union->task;
}

static task_struct* create_task_from_elf(const char *file_path)
{
    task_struct* task = create_task(0, USER_CODE_SELECTOR, USER_DATA_SELECTOR);
    if (task == NULL)
    {
        return NULL;
    }
    // 加载 ELF 文件
    file_struct file;
    file_open(file_path, &file);
    task->entry = elf_loader(task->page_dir, &file);

    return task;
}

static void switch_to_task(task_struct *task)
{
    DEBUGK("switch to task %d", task->pid);
    
    current_task = task;
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

void first_task(void)
{
    DEBUGK("start running the first task");

    // 启用时钟中断
    start_timer();
    while (1)
    {
        // TODO: 主动触发调度器
    }
}

void task_init(void)
{
    for (int i = 0; i < NR_TASKS; i++)
    {
        task_unions[i].task.state = TASK_DEAD;
    }

    // 创建首个任务
    // 由于需要在首个任务中启用时钟中断，所以段选择子使用内核代码段和内核数据段
    task_struct *task = create_task((uint32_t)first_task, KER_CODE_SELECTOR, KER_DATA_SELECTOR);
    switch_to_task(task);
}

void timer_handler(interrupt_frame *stask_top)
{
    DEBUGK("timer interrupt");

    current_task->interrupt_frame = stask_top;

    // 发送中断结束信号
    pic_send_eoi(0);
}
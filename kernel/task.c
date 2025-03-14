#include "kernel/task.h"
#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/elf.h"
#include "kernel/timer.h"
#include "kernel/pic.h"
#include "kernel/x86.h"

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
} task_struct;

typedef union task_union
{
    task_struct task;
    uint8_t kernel_stack[PAGE_SIZE];
} task_union;

static task_union task_unions[NR_TASKS] __attribute__((aligned(PAGE_SIZE))) = {0};
static task_struct* current_task = NULL;

static task_struct* create_task(const char *file_path)
{
    static int _pid = 0;

    task_unions[_pid].task.pid = _pid;
    task_unions[_pid].task.tss.ss0 = segment_selector(KER_DATA_INDEX, 0, 0);
    task_unions[_pid].task.tss.esp0 = (uint32_t)&task_unions[_pid].kernel_stack[PAGE_SIZE];
    
    // 初始化进程页目录
    task_unions[_pid].task.page_dir = create_user_page_dir();
    // 加载 ELF 文件
    file_struct file;
    file_open(file_path, &file);
    task_unions[_pid].task.entry = elf_loader(task_unions[_pid].task.page_dir, &file);
    // 初始化进程用户态栈
    uint32_t stack_page = pmu_alloc();
    task_unions[_pid].task.usr_stack_top = PAGE_SIZE + map_physical_page(task_unions[_pid].task.page_dir, stack_page, 1, 1);
    // 初始化中断栈帧，模拟中断调用
    // NOTE: 不能将中断栈帧指向 kernel_stack 的开始处，
    //       因为这是一个 union，修改 kernel_stack 前面的部分会覆盖 task 结构体
    task_unions[_pid].task.interrupt_frame = (interrupt_frame *)(task_unions[_pid].kernel_stack + PAGE_SIZE - sizeof(interrupt_frame));
    task_unions[_pid].task.interrupt_frame->eip = (uint32_t)task_unions[_pid].task.entry;
    task_unions[_pid].task.interrupt_frame->cs = segment_selector(USER_CODE_INDEX, 3, 0);
    task_unions[_pid].task.interrupt_frame->eflags = get_eflags();
    task_unions[_pid].task.interrupt_frame->esp = task_unions[_pid].task.usr_stack_top;
    task_unions[_pid].task.interrupt_frame->ss = segment_selector(USER_DATA_INDEX, 3, 0);
    task_unions[_pid].task.interrupt_frame->ds = segment_selector(USER_DATA_INDEX, 3, 0);
    task_unions[_pid].task.interrupt_frame->es = segment_selector(USER_DATA_INDEX, 3, 0);
    task_unions[_pid].task.interrupt_frame->fs = segment_selector(USER_DATA_INDEX, 3, 0);
    task_unions[_pid].task.interrupt_frame->gs = segment_selector(USER_DATA_INDEX, 3, 0);

    return &task_unions[_pid++].task;
}

static void switch_to_task(task_struct *task)
{
    DEBUGK("switch to task %d\n", task->pid);
    current_task = task;
    // 切换 TSS
    set_tss(&task->tss);
    // 切换页目录
    switch_to_user_page(task->page_dir);
    BMB;
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
    task_struct* task = create_task("/hello.bin");
    switch_to_task(task);
}
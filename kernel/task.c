#include "kernel/task.h"
#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "kernel/elf.h"

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
    uint32_t stack_top;
    uint32_t entry;
    page_dir_entry *page_dir;
} task_struct;

typedef union task_union
{
    task_struct task;
    uint8_t kernel_stack[PAGE_SIZE];
} task_union;

static task_union task_unions[NR_TASKS] __attribute__((aligned(PAGE_SIZE))) = {0};

static task_struct* create_task(const char *file_path)
{
    static int _pid = 0;

    task_unions[_pid].task.pid = _pid;
    task_unions[_pid].task.tss.ss0 = segment_selector(KER_DATA_INDEX, 0, 0);
    task_unions[_pid].task.tss.esp0 = (uint32_t)&task_unions[_pid].kernel_stack[PAGE_SIZE];
    
    // 初始化进程页目录
    task_unions[_pid].task.page_dir = create_user_page_dir();
    // 初始化进程用户态栈
    uint32_t stack_page = pmu_alloc();
    task_unions[_pid].task.stack_top = PAGE_SIZE + map_physical_page(task_unions[_pid].task.page_dir, stack_page, 1, 1);
    
    file_struct file;
    file_open(file_path, &file);
    task_unions[_pid].task.entry = elf_loader(task_unions[_pid].task.page_dir, &file);

    return &task_unions[_pid++].task;
}

static void switch_to_task(const task_struct *task)
{
    // 切换 TSS
    set_tss(&task->tss);
    // 切换页目录
    set_cr3((uint32_t)task->page_dir);
    // 切换上下文
    asm volatile(
        "push %0\n" // SS
        "push %1\n" // ESP
        "pushf\n"   // EFLAGS
        "push %2\n" // CS
        "push %3\n" // EIP
        "iret"
        :
        : "i"(segment_selector(USER_DATA_INDEX, 3, 0)),
          "r"(task->stack_top),
          "i"(segment_selector(USER_CODE_INDEX, 3, 0)),
          "r"(task->entry)
        : "memory");
}

void task_init(void)
{
    task_struct* task = create_task("/hello.bin");
    switch_to_task(task);
}
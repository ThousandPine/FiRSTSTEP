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
static task_struct *init_task = NULL;

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

static int set_free_task_union(task_union *ptr)
{
    // FIXME: 可以不用循环查找，而是直接通过地址计算出索引
    for (int i = 0; i < NR_TASKS; i++)
    {
        if (ptr == &task_unions[i])
        {
            if (is_used[i] == 0)
            {
                DEBUGK("task union is already free");
                return -1;
            }
            is_used[i] = 0;
            return 0;
        }
    }
    DEBUGK("invalid task union");
    return -1;
}

static void init_task_stack(task_union *task_union, uint32_t entry)
{
    // 初始化进程用户态栈
    uint32_t stack_page = pmu_alloc();
    uint32_t maped_usr_stack_top = PAGE_SIZE + map_physical_page(task_union->task.page_dir, stack_page, 1, 1);
    // 初始化中断栈帧，模拟中断调用
    // NOTE: 不能将中断栈帧指向 kernel_stack 的开始处，
    //       因为这是一个 union，修改 kernel_stack 前面的部分会覆盖 task 结构体
    task_union->task.interrupt_frame = (interrupt_frame *)(task_union->kernel_stack + PAGE_SIZE - sizeof(interrupt_frame));
    memset(task_union->task.interrupt_frame, 0, sizeof(interrupt_frame));
    task_union->task.interrupt_frame->ss = USER_DATA_SELECTOR;
    task_union->task.interrupt_frame->esp = maped_usr_stack_top;
    task_union->task.interrupt_frame->eflags = get_eflags();
    task_union->task.interrupt_frame->cs = USER_CODE_SELECTOR;
    task_union->task.interrupt_frame->eip = entry;
    task_union->task.interrupt_frame->es = USER_DATA_SELECTOR;
    task_union->task.interrupt_frame->ds = USER_DATA_SELECTOR;
    task_union->task.interrupt_frame->fs = USER_DATA_SELECTOR;
    task_union->task.interrupt_frame->gs = USER_DATA_SELECTOR;
}

static task_struct* create_task(uint32_t entry, page_dir_entry *page_dir, task_struct *parent)
{
    assert(page_dir != NULL);

    static int _pid = INIT_PID;

    task_union* task_union = get_free_task_union();
    if (task_union == NULL)
    {
        DEBUGK("create task failed");
        return NULL;
    }

    // 设置父子、兄弟关系
    task_union->task.child = NULL;
    task_union->task.parent = parent;
    if (parent != NULL)
    {
        task_union->task.sibling = parent->child;
        parent->child = &task_union->task;
    }
    // 设置独特的 pid
    task_union->task.pid = _pid++;
    // 进程状态
    task_union->task.state = TASK_NONE;
    // 初始化 TSS ，设置内核态栈
    task_union->task.tss.ss0 = KER_DATA_SELECTOR;
    task_union->task.tss.esp0 = (uint32_t)&task_union->kernel_stack[PAGE_SIZE];
    // 初始化进程页目录
    task_union->task.page_dir = page_dir;
    // 初始化进程用户态栈和中断栈帧
    init_task_stack(task_union, entry);

    return &task_union->task;
}

/**
 * 释放申请的内存资源
 * 
 * 由于程序使用的内存页都映射到了页目录中
 * 所以只要释放整个页目录映射的内存与页目录本身
 * 不需要额外释放栈页
 */
static void free_task_alloced_memory(task_struct *task)
{
    assert(task != NULL);

    free_user_page_dir(task->page_dir);
    task->page_dir = NULL;
}

task_struct* create_task_from_elf(const char *file_path, task_struct *parent)
{
    // 创建页目录
    page_dir_entry* page_dir = create_user_page_dir();

    // 加载 ELF 文件，读取程序数据到内存，并获取程序入口
    uint32_t entry = elf_loader(page_dir, file_path);
    if (entry == 0)
    {
        DEBUGK("load ELF file failed");
        free_user_page_dir(page_dir);
        return NULL;
    }
    
    // 创建进程
    task_struct* task = create_task(entry, page_dir, parent);
    if (task == NULL)
    {
        free_user_page_dir(page_dir);
        return NULL;
    }

    return task;
}

int reload_task_from_elf(const char *file_path, task_struct *task)
{
    assert(task != NULL);
    assert(file_path != NULL);

    // 新建页目录
    page_dir_entry* page_dir = create_user_page_dir();

    // 加载 ELF 文件，读取程序数据到内存，并获取程序入口
    uint32_t entry = elf_loader(page_dir, file_path);
    if (entry == 0)
    {
        DEBUGK("load ELF file failed");
        free_user_page_dir(page_dir);
        return -1;
    }

    // 释放原有的页目录
    free_user_page_dir(task->page_dir);
    task->page_dir = page_dir;

    // 重置栈和中断栈帧，设置任务返回地址
    init_task_stack((task_union *)task, entry);

    return 0;
}

task_struct* fork_task(task_struct *parent)
{
    assert(parent != NULL);

    page_dir_entry *page_dir = create_user_page_dir();
    // 拷贝页目录与内存数据
    if(copy_page_dir_and_memory(page_dir, parent->page_dir) < 0)
    {
        DEBUGK("copy task failed");
        free_user_page_dir(page_dir);
        return NULL;
    }
    
    // 创建新任务
    task_struct* new_task = create_task(0, page_dir, parent);
    if (new_task == NULL)
    {
        free_user_page_dir(page_dir);
        return NULL;
    }

    // 拷贝中断上下文，得到返回地址，栈顶指针等信息
    *new_task->interrupt_frame = *parent->interrupt_frame;

    return new_task;
}

/**
 * 
 * NOTE: 进程退出时不会立刻释放所有资源，而是会保留进程结构体，用于记录退出状态。
 *       直到父进程调用 wait() 或 waitpid() 获取子进程的退出状态，资源才会被完全回收。
 *       这种已经退出但资源还未被回收的进程，被称为“僵尸进程”
 */
void task_exit(task_struct *task, int exit_code)
{
    assert(task != NULL)

    if (task == init_task)
    {
        panic("init task exit");
    }

    task->exit_code = exit_code;
    
    // 释放申请的内存资源
    free_task_alloced_memory(task);

    // 子进程变为孤儿进程, 由 init 进程接管
    task_struct *last_child = NULL;
    for (task_struct *child = task->child; child != NULL; child = child->sibling) 
    {
        child->parent = init_task;
        last_child = child;
    }
    // 添加到init 进程的子进程列表开头
    if (last_child != NULL)
    {
        last_child->sibling = init_task->child;
        init_task->child = task->child;
    }
}

void task_dead(task_struct *task)
{
    assert(task != NULL);

    // 释放 task_union
    set_free_task_union((task_union *)task);
}

void task_init(void)
{
    // 创建首个任务
    init_task = create_task_from_elf("/bin/init", NULL);

    if (init_task == NULL)
    {
        panic("create init task failed");
    }
    
    // 初始化调度器
    scheduler_init(init_task);
}

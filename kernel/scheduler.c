#include "kernel/scheduler.h"
#include "kernel/timer.h"
#include "kernel/pic.h"
#include "kernel/kernel.h"

typedef struct task_list
{
    task_struct *head;
    task_struct *tail;
} task_list;

static task_struct *current_task = NULL;
static task_list ready_tasks = {NULL, NULL};
static task_list blocked_tasks = {NULL, NULL};

/**
 * 切换到任务执行上下文
 */
static void context_switch_to(task_struct *task)
{
    DEBUGK("switch to task %d", task->pid);

    // 切换 TSS
    set_tss(&task->tss);
    // 切换页目录
    switch_page_dir(task->page_dir);
    // 切换上下文
    asm volatile(
        // 定位中断栈帧
        "mov %0, %%esp\n"
        // 恢复上下文
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

static void task_list_add(task_list *list, task_struct *task)
{
    task->prev = task->next = NULL;
    if (list->head == NULL)
    {
        list->head = list->tail = task;
        return;
    }

    task->prev = list->tail;
    list->tail->next = task;
    list->tail = task;
}

static int task_list_remove(task_list *list, task_struct *target)
{
    // 遍历链表找到目标
    task_struct *task = list->head;
    while (task != NULL && task != target)
    {
        task = task->next;
    }

    if (task == NULL)
    {
        return -1;
    }

    // 修改头尾节点
    if (task == list->head)
    {
        list->head = task->next;
    }
    if (task == list->tail)
    {
        list->tail = task->prev;
    }

    // 从链表中移除节点
    task->prev->next = task->next;
    task->next->prev = task->prev;
    task->prev = task->next = NULL;

    return 0;
}

static task_struct *get_next_ready_task(void)
{
    // TODO: 实现调度算法
    return ready_tasks.head;
}

task_struct *running_task(uint8_t not_null)
{
    assert(not_null == 0 || current_task != NULL);
    return current_task;
}

static void switch_to_running_state(task_struct *task)
{
    switch (task->state)
    {
    case TASK_READY:
        task_list_remove(&ready_tasks, task);
        break;

    default:
        panic("invalid task state %d switch to running state", task->state);
        break;
    }

    // 将正在执行的任务状态设为 READY
    if (running_task(0) != NULL)
    {
        switch_task_state(running_task(1), TASK_READY);
    }

    // 执行任务
    task->state = TASK_RUNNING;
    current_task = task;
    context_switch_to(task);
}

static void switch_to_ready_state(task_struct *task)
{
    switch (task->state)
    {
    case TASK_NONE:
        break;
    case TASK_RUNNING:
        current_task = NULL;
        break;

    default:
        panic("invalid task state %d switch to ready state", task->state);
        break;
    }

    task->state = TASK_READY;
    task_list_add(&ready_tasks, task);
}

static void switch_to_zombie_state(task_struct *task)
{
    switch (task->state)
    {
    case TASK_RUNNING:
        current_task = NULL;
        break;

    default:
        panic("invalid task state %d switch to zombie state", task->state);
        break;
    }

    task->state = TASK_ZOMBIE;
}

static void switch_to_dead_state(task_struct *task)
{
    switch (task->state)
    {
    case TASK_ZOMBIE:
        break;

    default:
        panic("invalid task state %d switch to dead state", task->state);
        break;
    }

    task->state = TASK_DEAD;
    task_dead(task);
}

void switch_task_state(task_struct *task, enum task_state state)
{
    if (task->state == state)
    {
        return;
    }

    switch (state)
    {
    case TASK_RUNNING:
        switch_to_running_state(task);
        break;
    case TASK_READY:
        switch_to_ready_state(task);
        break;
    case TASK_ZOMBIE:
        switch_to_zombie_state(task);
        break;
    case TASK_DEAD:
        switch_to_dead_state(task);
        break;
    default:
        panic("invalid task state %d to switch", state);
    }
}

/**
 * 调度下一个任务
 * 
 * NOTE: 则该函数不会返回
 */
void schedule_handler(interrupt_frame *frame)
{
    // 保存当前任务的中断栈帧
    if (current_task != NULL)
    {
        current_task->interrupt_frame = frame;
    }

    // 获取下一个任务
    task_struct *next_task = get_next_ready_task();
    if (next_task == NULL)
    {
        // 未找到任务，则继续调度当前任务
        context_switch_to(running_task(1));
    }

    // 切换到下一个任务
    switch_task_state(next_task, TASK_RUNNING);
}

void scheduler_init(task_struct *init_task)
{
    // 添加初始任务到调度队列
    switch_task_state(init_task, TASK_READY);
    // 启用时钟中断
    start_timer();

    // 让 CPU 进入休眠状态，等待时钟中断
    while (1)
    {
        asm volatile("hlt");
    }
}
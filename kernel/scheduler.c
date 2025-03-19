#include "kernel/scheduler.h"
#include "kernel/timer.h"
#include "kernel/pic.h"

task_struct *current_task = NULL;
static task_struct *idle_task = NULL;

static task_struct *get_next_task(void)
{
    // TODO: 实现调度算法
    if (idle_task == NULL)
    {
        return NULL;
    }
    task_struct *next_task = idle_task;
    idle_task = idle_task->next;
    next_task->prev = next_task->next = NULL;
    return next_task;
}

// FIXME: 防止添加任务时被中断打断，导致链表操作不完整
void scheduler_add_task(task_struct *task)
{
    // FIXME: 临时代码
    if (idle_task == NULL)
    {
        idle_task = task;
        return;
    }
    for (task_struct *p = idle_task; p != NULL; p = p->next)
    {
        if (p->next == NULL)
        {
            p->next = task;
            task->prev = p;
            task->next = NULL;
            break;
        }
    }
}

void schedule(interrupt_frame *frame)
{
    // 保存当前任务的中断栈帧
    if (current_task != NULL)
    {
        current_task->interrupt_frame = frame;
    }

    // 获取下一个任务
    task_struct *next_task = get_next_task();
    if (next_task == NULL)
    {
        return;
    }
    scheduler_add_task(current_task);
    current_task = next_task;

    // 切换任务前，需要通知时钟中断处理完毕，否则无法开始下一次时钟中断
    handled_timer();
    // 切换到下一个任务
    switch_to_task(next_task);
}

void scheduler_init(task_struct *task)
{
    // 添加初始任务到调度队列
    scheduler_add_task(task);
    // 启用时钟中断
    start_timer();
}
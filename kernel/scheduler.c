#include "kernel/scheduler.h"
#include "kernel/timer.h"
#include "kernel/pic.h"

static task_struct *idle_task = NULL;
static task_struct *current_task = NULL;

static task_struct *get_next_task(void)
{
    // TODO: 实现调度算法

    return idle_task; // FIXME: 临时代码
}

// FIXME: 防止添加任务时被中断打断，导致链表操作不完整
void scheduler_add_task(task_struct *task)
{
    // TODO: 添加任务到调度队列

    idle_task = task; // FIXME: 临时代码
}

void schedule(interrupt_frame *frame)
{
    // 保存当前任务的中断栈帧
    if (current_task != NULL)
    {
        current_task->interrupt_frame = frame;
    }

    // 获取下一个任务
    current_task = get_next_task();

    // 切换任务前，需要通知时钟中断处理完毕，否则无法开始下一次时钟中断
    handled_timer();
    // 切换到下一个任务
    switch_to_task(current_task);
}

void scheduler_init(task_struct *task)
{
    // 添加初始任务到调度队列
    scheduler_add_task(task);
    // 启用时钟中断
    start_timer();
}
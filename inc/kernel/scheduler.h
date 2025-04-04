#pragma once

#include "types.h"
#include "kernel/task.h"

void scheduler_init(task_struct *init_task);
void schedule(void);
void schedule_handler(interrupt_frame *frame);
task_struct *running_task(void);
void switch_task_state(task_struct *task, enum task_state state);
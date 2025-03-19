#pragma once

#include "types.h"
#include "kernel/task.h"

extern task_struct *current_task;

void scheduler_init(task_struct *task);
void schedule(interrupt_frame *frame);
void scheduler_add_task(task_struct *task);
#pragma once

#include "types.h"
#include "kernel/page.h"
#include "kernel/gdt.h"

#define NR_TASKS 100 // 最大任务数量

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

task_struct* create_task_from_elf(const char *file_path);
void switch_to_task(task_struct *task);
task_struct* copy_task(const task_struct *task);
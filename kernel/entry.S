#include "kernel/memlayout.h"

#define STACK_SIZE 4096

.code32
.global _start
.extern init, page_init

.section .lower.text
_start:
    cli

    # 栈定义在 .bss 段，还没有启用 Higher Half Kernel 的情况下需要减去偏移量
    mov    $(stack + STACK_SIZE - HIGHER_HALF_KERNEL_BASE), %esp
    mov    $0, %ebp # 清除栈帧

    # 设置分页，将内核映射到高地址
    call   page_init

    jmp    higherhalf

.section .lower.data



.section .text
higherhalf:
    mov    $(stack + STACK_SIZE), %esp
    call   init
1:
    hlt
    jmp    1b

.section .bss
.lcomm stack, STACK_SIZE
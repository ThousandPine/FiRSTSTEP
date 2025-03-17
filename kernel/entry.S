#define STACK_SIZE 4096

.global _start, isr_timer, isr_syscall
.extern gdt_init, init, timer_handler, syscall_handler

# 在 .bss 段定义栈空间
.section .bss
.lcomm stack, STACK_SIZE

.section .text
.code32
# 内核入口
_start:
    cli
    mov     $(stack + STACK_SIZE), %esp # 重新设置栈指针
    call    gdt_init
    call    init
    hlt

# 时钟中断服务
isr_timer:
    pusha
    push    %ds
    push    %es
    push    %fs
    push    %gs

    push    %esp            # 当前栈指针作为函数参数压栈
    call    timer_handler
    add     $4, %esp

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    iret

# 系统调用中断服务
isr_syscall:
    pusha
    push    %ds
    push    %es
    push    %fs
    push    %gs

    push    %esp    # 中断栈帧
    push    %edx    # 系统调用参数 3
    push    %ecx    # 系统调用参数 2
    push    %ebx    # 系统调用参数 1
    push    %eax    # 系统调用号
    call    syscall_handler
    add     $(4 * 5), %esp

    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    popa
    iret
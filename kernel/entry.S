#define STACK_SIZE 4096

.global _start, schedule, isr_timer, isr_syscall
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

# 手动触发调度
schedule:
    # 模仿 x86 中断的栈帧结构
    # 由于该函数是在系统态下调用的，不存在权限切换
    # 所以中断栈帧不包含 ss 和 esp
    xchgw %bx, %bx
    pushf                      # EFLAGS
    push    %cs
    push    $schedule_recovery # 返回地址

    pusha
    push    %ds
    push    %es
    push    %fs
    push    %gs

    push    %esp             # 当前栈指针作为函数参数压栈
    call    schedule_handler # 通过调度器触发任务切换，该函数不会返回
schedule_recovery:
    ret

# 时钟中断服务
isr_timer:
    pusha
    push    %ds
    push    %es
    push    %fs
    push    %gs

    push    %esp            # 当前栈指针作为函数参数压栈
    call    timer_handler   # 如果触发了任务切换，就不会从此处返回
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
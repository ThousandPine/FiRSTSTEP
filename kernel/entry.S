#define STACK_SIZE 4096

.global _start
.extern init

# 在 .bss 段定义栈空间
.section .bss
.lcomm stack, STACK_SIZE

.section .text
.code32
    _start:
    cli
    mov     $(stack + STACK_SIZE), %esp
    call    init
    hlt


#define STACK_SIZE 4096

.code32
.global _start
.extern init

.section .text.lower
_start:
    cli
    lgdt    trickgdt
    ljmp    $0x08, $higherhalf

.section .data.lower
# 将代码段和数据段的 base 设为 0x40000000，这样后续访问 0xC0000000 (3 GiB)
# 以上的偏移地址时，就会自动回环到地址 0x0 以上，使偏移地址与物理地址对等。
.align 4
gdt:
    .long   0, 0                                              # null seg
    .byte   0xFF, 0xFF, 0, 0, 0, 0b10011010, 0b11001111, 0x40   # code seg; base 0x40000000, limit 0xFFFFFFFF
    .byte   0xFF, 0xFF, 0, 0, 0, 0b10010010, 0b11001111, 0x40   # data seg; base 0x40000000, limit 0xFFFFFFFF
gdt_end:

trickgdt:
    .word   gdt_end - gdt - 1       # sizeof(gdt) - 1
    .long   gdt                     # address gdt


.section .text
higherhalf:
    mov    $0x10, %ax
    mov    %ax, %ds
    mov    %ax, %es
    mov    %ax, %fs
    mov    %ax, %gs
    mov    %ax, %ss
    mov    $(stack + STACK_SIZE), %esp
    call   init
    hlt

.section .bss
.lcomm stack, STACK_SIZE
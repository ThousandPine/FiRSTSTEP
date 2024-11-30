#include "boot.h"

.section .text
.global _start

    # 在程序开头使用 1 字节记录 setup 程序占用的扇区数量
    # bootsect 程序会根据该记录，从硬盘加载对应大小的数据到内存
    # 跳转到入口时则需要额外偏移 1 字节，以免把该数据当作代码执行
    # 该数值由 write_setup.sh 脚本计算并写入
    .byte   0

    _start:
    .code16

    # 设置段地址为 0x0800
    mov $SETUP_SEG, %ax
    mov %ax, %ds
    mov %ax, %es

    # 输出运行信息
    mov    $msg0, %si  # SI 指向字符串起始地址
    call   print       # 调用打印字符串子程序

    # TODO: BIOS 中断读取并保存硬件信息

    # 出于旧程序的兼容性，A20 地址线被默认关闭（始终为 0）
    # 因此需要启用 A20 地址线，确保程序正常访问 1MB 以上的空间
    call    a20wait
    mov     $0xd1, %al
    out     %al, $0x64   # 0x64 端口设为 0xd1，表示向 8042 芯片写数据
    call    a20wait
    mov     $0xdf, %al
    out     %al, $0x60   # 将启用 A20 对应的 bit 设为 1
    call    a20wait

    # TODO: 开启保护模式

    # FIXME: 跳转到 32 位代码
    # ljmp    xxx, $protcseg
    hlt

# 等待，直到键盘输入队列为空
a20wait:
    in      $0x64,%al
    test    $0x2,%al
    jnz     a20wait
    ret

# 打印字符串
#
# @param SI 字符串内存地址
#
# NOTE: 使用 BIOS 中断实现，因此只能在实模式使用
print:
    mov    $0x0E, %ah  # BIOS 功能号：显示字符
    mov    $0x00, %bh  # 页面号
    mov    $0x07, %bl  # 文字属性（白色）
.print_loop:
    lodsb              # 从 [SI] 加载下一个字符到 AL
    cmp    $0, %al     # 判断是否到字符串末尾
    je     .print_done # 如果是，则结束
    int    $0x10       # 调用 BIOS 中断
    jmp    .print_loop # 继续打印下一个字符
.print_done:
    ret

msg0:
    .asciz "Running setup ...\n\r"

    .code32
protcseg:
    # 设置段寄存器

    # 设置堆栈

    # 测试 A20 gate

    # 读取活动分区的 FAT32 文件系统根目录
    # 找到内核程序，并加载到 0x100000

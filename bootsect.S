.section .text
.global _start
_start:
    .code16         # 16 位模式代码
    cli             # 关闭中断
    cld             # 清除方向标志位

    # 初始化段寄存器
    xorw    %ax, %ax
    movw    %ax, %ds    # 数据段
    movw    %ax, %ss    # 堆栈段
    movw    %ax, %es    # 额外段

    # 打印信息
    movw    $msg, %si   # SI 指向字符串起始地址
    call    print       # 调用打印字符串子程序
    hlt

print:
    movb    $0x0E, %ah  # BIOS 功能号：显示字符
    movb    $0x00, %bh  # 页面号
    movb    $0x07, %bl  # 文字属性（白色）
print_loop:
    lodsb               # 从 [SI] 加载下一个字符到 AL
    cmp     $0, %al     # 判断是否到字符串末尾
    je      done        # 如果是，则结束
    int     $0x10       # 调用 BIOS 中断
    jmp     print_loop  # 继续打印下一个字符
done:
    ret

msg:
    .asciz "Running bootsect ..."

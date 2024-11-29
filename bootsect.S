.equ SETUP_SEG, 0x0800 # 0x8000 对应的段是 0x0800

.section .text
.global _start
_start:
    .code16         # 16 位模式代码
    cli             # 禁止外部中断
    cld             # 清除方向标志位

    # 初始化段寄存器
    xor    %ax, %ax
    mov    %ax, %ds         # 数据段
    mov    %ax, %ss         # 堆栈段
    mov    $SETUP_SEG, %ax
    mov    %ax, %es         # 额外段

    # 由于栈向低地址增长，所以设栈顶为程序起始处 (0x7c00)
    # 相当于把引导程序下方的空间作为栈
    mov    $_start, %sp

    mov    $msg, %si   # SI 指向字符串起始地址
    call   print       # 调用打印字符串子程序

    # 将保存在 MBR gap 的 setup 程序读取到 0x8000 处
    # 程序开头 1 字节表示占用的扇区数

    # 接下来要将 setup 程序读取到内存 0x8000 处
    # 程序保存硬盘第二扇区开始的连续空间中 (MBR gap)
    # 第二扇区开头的 1 字节表示程序占用的扇区数
    # 因此，我们先读取第二扇区，获取程序大小
    # 接着，将对应数量的扇区读入 0x8000 之中
    # 最后，jmp 0x8001（跳过开头表示扇区数的那 1 字节）执行 setup 程序
    mov    $1, %ax
    mov    $1, %cl
    mov    $0, %di
    call   read_sectors

    mov    $1, %ax
    mov    0, %cl      # 0 表示段内地址，mov 使用 es 段寄存器，所以是 %es:0，也就是 0x8000
    mov    $0, %di
    call   read_sectors

    jmp   (SETUP_SEG << 4 + 1) # 跳过开头表示扇区数量的 1 字节

# ATA PIO 读取硬盘扇区 (16 位版本)
# 使用 LBA28 寻址模式 
# 
# @param AX 起始 LBA 序号
# @param CL 扇区数量
# @param DI 保存数据的内存地址
# 
# FIXME: 数据过多可能会导致 DI 回环，覆盖其他区域的数据
# FIXME: 缺少寄存器状态保存与恢复
# FIXME: 缺少硬盘 BSY 和 ERR 状态判断
# FIXME: 缺少硬盘状态判断前的 400ns 延迟 https://wiki.osdev.org/ATA_PIO_Mode#400ns_delays
read_sectors:
    mov    %ax, %bx    # 暂存参数

    mov    $0x1F2, %dx # 读写扇区数量的端口
    mov    %cl, %al    # 读取扇区数量
    out    %al, %dx    # 数据写入端口

    mov    $0x1F3, %dx # 设置 LBAlow 的端口
    mov    %bx, %ax    # LBAlow
    out    %al, %dx    # 数据写入端口

    mov    $0x1F4, %dx # 设置 LBAmid 的端口
    mov    %bh, %al    # LBAmid
    out    %al, %dx    # 数据写入端口
    
    mov    $0x1F5, %dx # 设置 LBAhi 的端口
    mov    $0, %al     # 由于传参的 ax 只有 16 位，因此 LBAhi 为 0
    out    %al, %dx    # 数据写入端口

    mov    $0x1F6, %dx      # 设置驱动与 LBA 高四位的端口
    mov    $0b11100000, %al # 1110 表示使用主硬盘和 LBA 寻址；0000 则是 LBA 24 ~ 27 位的值
    out    %al, %dx         # 数据写入端口

    mov    $0x1F7, %dx # 设置写入命令的端口
    mov    $0x20, %ax  # 设置读取数据命令
    out    %al, %dx

    mov    %cl, %bl    # 将扇区计数保存在 bl
.still_going:
    mov    $0x1F7, %dx # 设置读取状态的端口
    in     %dx, %al    # 读取硬盘状态
    test   $8, %al     # 测试 DRQ 位是否置 1
    jz     .still_going
    # 读取数据
    mov    $256, %cx   # 一次读取 2 字节，一个扇区读取 256 次
    mov    $0x1F0, %dx # 设置数据读写端口
    rep    insw        # 读取一个扇区
    # 是否读取下一个扇区
    dec    %bl
    jnz    .still_going
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

msg:
    .asciz "Running bootsect ...\n\r"

# 填充到 446 字节上限
.org 446
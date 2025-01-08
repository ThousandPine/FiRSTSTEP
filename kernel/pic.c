#include "kernel/x86.h"
#include "kernel/idt.h"

/**
 * 硬件上通过串联两个 8259A 芯片实现 15 种外部中断识别
 * 两个 8259A 芯片分别称为主片（连接 CPU）和从片（连接主片）
 * 使用四个端口号分别控制两个芯片的功能
 */
#define PIC1_IO 0x20 // IO base address for master PIC
#define PIC2_IO 0xA0 // IO base address for slave PIC

// Initialization Command Words
#define ICW1_ICW4 0x11 // 表示需要 ICW4

// 输出到端口，并等待一段时间
static void out_delay(uint16_t prot, uint8_t value)
{
    outb(prot, value);
    // 给 PIC 一些时间响应命令
    for (int i = 10; i > 0; i--)
    {
        asm volatile("");
    }
}

// 初始化 8259A 中断控制芯片
void pic_init(void)
{
    /**
     * 保护模式下 0-31 号的中断向量为 CPU 保留项
     * 而默认情况下 IRQ 0-7 映射的中断向量为 8-15
     * 为了避免冲突，需要重新映射 PIC 到非保留区域中
     *
     * 重新映射 PIC 的唯一方法是重新初始化 8259A 芯片
     * 需要向端口写入多个 ICW (Initialization Command Words) 数据
     * 包括 ICW1 ICW2 ICW3 ICW4 ，每个 ICW 的数据定义都不相同
     * 具体说明可以参考：
     * https://helppc.netcore2k.net/hardware/8259
     * https://pdos.csail.mit.edu/6.828/2005/readings/hardware/8259A.pdf
     */

    // 发送 ICW1 表示开始初始化
    out_delay(PIC1_IO, ICW1_ICW4); // 主片 ICW1
    out_delay(PIC2_IO, ICW1_ICW4); // 从片 ICW1

    // 通过 ICW2 设置 IRQ 映射的中断向量起始编号
    out_delay(PIC1_IO + 1, IDT_PIC1_OFFSET); // 主片 ICW2
    out_delay(PIC2_IO + 1, IDT_PIC2_OFFSET); // 从片 ICW2

    // 设置 ICW3
    out_delay(PIC1_IO + 1, 4); // 主片 ICW3 ，通过设置第 3 位表示从片连接在 IRQ2
    out_delay(PIC2_IO + 1, 2); // 从片 ICW3 ，使用数字 2 表示从片连接在 IRQ2

    // 通过 ICW4 设置为 80x86 模式
    out_delay(PIC1_IO + 1, 1); // 主片 ICW4
    out_delay(PIC2_IO + 1, 1); // 从片 ICW4
}
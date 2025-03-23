#include "kernel/idt.h"
#include "kernel/gdt.h"
#include "kernel/isr.h"
#include "kernel/x86.h"
#include "kernel/kernel.h"

// 中断向量表
static gate_descriptor idt[IDT_ENTRY_COUNT] = {0};

void set_gate(size_t index, uint8_t type, void *addr)
{
    idt[index].gate_type = type;
    idt[index].offset_lo = (uint32_t)addr & 0xFFFF;
    idt[index].offset_hi = ((uint32_t)addr >> 16) & 0xFFFF;
    idt[index].selector = KER_CODE_SELECTOR;
    idt[index].dpl = 0;
    idt[index].present = 1;
    idt[index]._zero = 0;
}

// 初始化中断向量表
void idt_init()
{
    // 中断号 0-31 由 CPU 保留用于处理各种异常情况
    set_gate(0, GT_INT, &isr_de);
    set_gate(1, GT_INT, &isr_db);
    set_gate(2, GT_INT, &isr_nmi);
    set_gate(3, GT_INT, &isr_bp);
    set_gate(4, GT_INT, &isr_of);
    set_gate(5, GT_INT, &isr_br);
    set_gate(6, GT_INT, &isr_ud);
    set_gate(7, GT_INT, &isr_nm);
    set_gate(8, GT_INT, &isr_df);
    set_gate(9, GT_INT, &isr_cso);
    set_gate(10, GT_INT, &isr_ts);
    set_gate(11, GT_INT, &isr_np);
    set_gate(12, GT_INT, &isr_ss);
    set_gate(13, GT_INT, &isr_gp);
    set_gate(14, GT_INT, &isr_pf);
    set_gate(15, GT_INT, &isr_reserved);
    set_gate(16, GT_INT, &isr_mf);
    set_gate(17, GT_INT, &isr_ac);
    set_gate(18, GT_INT, &isr_mc);
    set_gate(19, GT_INT, &isr_xm);
    set_gate(20, GT_INT, &isr_ve);
    set_gate(21, GT_INT, &isr_cp);
    // 22-31 为未使用的保留部分
    for (size_t i = 22; i < 32; i++)
    {
        set_gate(i, GT_INT, &isr_reserved);
    }

    // 32-255 为用户定义中断，先设置为默认中断服务程序
    for (size_t i = 32; i < IDT_ENTRY_COUNT; i++)
    {
        set_gate(i, GT_INT, &isr_default);
    }

    // 设置时钟中断服务
    set_gate(IDT_PIC1_OFFSET, GT_INT, &isr_timer);
    // 设置 IRQ7 和 IRQ15 的虚假中断处理
    set_gate(IDT_PIC1_OFFSET + 7, GT_INT, &isr_spurious_irq);
    set_gate(IDT_PIC2_OFFSET + 7, GT_INT, &isr_spurious_irq);
    
    // 设置系统调用中断
    set_gate(0x80, GT_INT, &isr_syscall);
    // 由于是用户态主动调用，所以要将 DPL 设为 3
    idt[0x80].dpl = 3;

    // 设置 IDTR
    idt_descriptor idtr = {
        .size = sizeof(idt) - 1,
        .offset = (uint32_t)idt,
    };
    // 加载 IDT
    asm volatile("lidt %0" ::"m"(idtr) : "memory");
}
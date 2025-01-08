#include "kernel/idt.h"
#include "kernel/gdt.h"
#include "kernel/kernel.h"

// 中断向量表
static GateDescriptor idt[IDT_ENTRY_COUNT] = {0};

// FIXME: 临时中断函数
static void temp_int_func(void)
{
    panic("Enter the interrupt function");
}

// 初始化中断向量表
void idt_init()
{
    // FIXME: 初始化 IDT 内容
    GateDescriptor gd = {
        .gate_type = GT_INT,
        .offset_lo = (uint32_t)temp_int_func & 0xFFFF,
        .offset_hi = ((uint32_t)temp_int_func >> 16) & 0xFFFF,
        .selector = seg_sel_val(KER_CODE_INDEX, 0, 0),
        .dpl = 0,
        .present = 1,
        ._zero = 0,
    };
    for (size_t i = 0; i < IDT_ENTRY_COUNT; i++)
    {
        idt[i] = gd;
    }


    // 设置 IDTR
    IDTDescriptor idtr = {
        .size = sizeof(idt),
        .offset = (uint32_t)idt,
    };
    // 加载 IDT
    asm volatile("lidt %0" ::"m"(idtr) : "memory");
}
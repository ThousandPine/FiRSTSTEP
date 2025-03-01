#pragma once

#include "types.h"

#define IDT_ENTRY_COUNT 256 // 有 256 个中断向量

// 0-31 为 CPU 保留中断，所以从 32 开始编号
#define IDT_PIC1_OFFSET 32 // IRQ 0-7 中断向量起始编号
#define IDT_PIC2_OFFSET 40 // IRQ 8-15 中断向量起始编号

// Gate Descriptor gate_type
#define GT_INT 0b1110  // 32-bit Interrupt Gate
#define GT_TRAP 0b1111 // 32-bit Trap Gate

// 门描述符，作为 IDT 的条目
typedef struct gate_descriptor
{
    uint16_t offset_lo;
    uint16_t selector;     // 段选择子
    uint8_t reserved;      // 保留位，无功能
    uint8_t gate_type : 4; // 门类型
    uint8_t _zero : 1;     // 始终为 0
    uint8_t dpl : 2;       // 权限等级 DPL
    uint8_t present : 1;   // 是否有效
    uint16_t offset_hi;

} __attribute__((packed)) gate_descriptor;

// 存储在 IDTR 中的数据，描述 IDT 的大小和起始位置
typedef struct idt_descriptor
{
    uint16_t size;   // 等于 IDT 的字节大小减去 1
    uint32_t offset; // IDT 的线性地址（不是物理地址，适用分页地址转换）
} __attribute__((packed)) idt_descriptor;

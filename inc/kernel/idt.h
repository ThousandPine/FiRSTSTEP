#pragma once

#include "types.h"

#define IDT_ENTRY_COUNT 256 // 有 256 个中断向量

// Gate Descriptor gate_type
#define GT_INT 0b1110  // 32-bit Interrupt Gate
#define GT_TRAP 0b1111 // 32-bit Trap Gate

// 门描述符，作为 IDT 的条目
typedef struct GateDescriptor
{
    uint16_t offset_lo;
    uint16_t selector;     // 段选择子
    uint8_t reserved;      // 保留位，无功能
    uint8_t gate_type : 4; // 门类型
    uint8_t _zero : 1;     // 始终为 0
    uint8_t dpl : 2;       // 权限等级 DPL
    uint8_t present : 1;   // 是否有效
    uint16_t offset_hi;

} __attribute__((packed)) GateDescriptor;

// 存储在 IDTR 中的数据，描述 IDT 的大小和起始位置
typedef struct IDTDescriptor
{
    uint16_t size;   // IDT 的字节大小
    uint32_t offset; // IDT 的线性地址（不是物理地址，适用分页地址转换）
} __attribute__((packed)) IDTDescriptor;

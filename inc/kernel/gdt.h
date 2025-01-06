#pragma once

#include "types.h"

#define GDT_ENTRY_COUNT 5 // GDT 条目数量
#define KER_CODE_INDEX 1  // GDT 内核代码段索引
#define KER_DATA_INDEX 2  // GDT 内核数据段索引
#define USER_CODE_INDEX 3 // GDT 用户代码段索引
#define USER_DATA_INDEX 4 // GDT 用户数据段索引

#define DA_DR 0b1000  // 只读数据段
#define DA_DRW 0b1001 // 可读写数据段
#define DA_C 0b1100   // 仅执行代码段
#define DA_CR 0b1101  // 可读代码段
#define DA_CC 0b1110  // 仅执行一致代码段
#define DA_CCR 0b1111 // 可读一致代码段

// 段描述符，作为 GDT 或 LDT 的条目
typedef struct SegmentDescriptor
{
    uint16_t lim_low;
    uint32_t base_low : 24;
    uint8_t accessed : 1; // 访问位，CPU会在访问该段时置 1
    uint8_t type : 4;     // 段类型，包括 S / E / DC / RW
    uint8_t dpl : 2;      // 权限等级 DPL
    uint8_t present : 1;  // 是否存在
    uint8_t lim_hi : 4;
    uint8_t reserved : 1;    // 保留位，无功能
    uint8_t long_mode : 1;   // 64 位标志
    uint8_t m32 : 1;         // 32 位标志
    uint8_t granularity : 1; // 启用 4KB 粒度地址
    uint8_t base_hi;
} __attribute__((packed)) SegmentDescriptor;

// 存储在 GDTR 中的数据，描述 GDT 的大小和起始位置
typedef struct GDTDescriptor
{
    uint16_t size;   // 等于 GDT 的字节大小减去 1。因为 size 的最大值为 65535，而 GDT 的长度最多为 65536 字节（8192 个条目）
    uint32_t offset; // GDT的线性地址（不是物理地址，适用分页地址转换）
} __attribute__((packed)) GDTDescriptor;

/**
 * 段选择子是（Segment Selector）存储在段寄存器中的数据
 * 该宏定义用于计算段选择子对应的 16bit 整数值
 *
 * 因为 gcc 的 ljmp 仅能使用立即数表示段选择子
 * 所以没有使用结构体，而是用宏定义计算出对应的立即数
 *
 * @param index GDT 或 LDT 条目索引
 * @param rpl 请求权限等级
 * @param ti 0 表示 GDT，1 表示 LDT
 */
#define seg_sel_val(index, rpl, ti) (uint16_t)(((rpl) & 0x3) | (((ti) << 2) & 0x4) | (((index) << 3) & 0xFFF8))

void gdt_init(void);
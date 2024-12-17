#pragma once

#include "types.h"

// 定义单个分区表项结构
struct PartitionEntry
{
    uint8_t boot_indicator; // 引导标志，0x80 表示可引导，0x00 表示不可引导
    uint8_t start_chs[3];   // 分区起始地址（CHS 格式）
    uint8_t partition_type; // 分区类型代码
    uint8_t end_chs[3];     // 分区结束地址（CHS 格式）
    uint32_t start_lba;     // 分区起始扇区（LBA 格式）
    uint32_t num_sectors;   // 分区占用的扇区总数
} __attribute__((packed));

// 定义 MBR 结构
struct MBR
{
    uint8_t boot_code[446];              // 引导代码区
    struct PartitionEntry partitions[4]; // 4 个分区表项
    uint16_t signature;                  // MBR 签名，必须是 0xAA55
} __attribute__((packed));
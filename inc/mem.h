// Descriptor Access Byte
#define DA_P    0x80 // 有效段
#define DA_A    0x01 // 访问位
#define DA_DPL0 0x00 // DPL = 0
#define DA_DPL1 0x20 // DPL = 1
#define DA_DPL2 0x40 // DPL = 2
#define DA_DPL3 0x60 // DPL = 3
// 存储段
#define DA_DR   0x10 // 只读数据段
#define DA_DRW  0x12 // 可读写数据段
#define DA_C    0x18 // 仅执行代码段
#define DA_CR   0x1A // 可读代码段
#define DA_CC   0x1C // 仅执行一致代码段
#define DA_CCR  0x1E // 可读一致代码段
// 系统段
#define DA_LDT      0x02 // 局部描述符表段
#define DA_TaskGate 0x05 // 任务门
#define DA_386TSS   0x09 // 可用 386 任务状态段
#define DA_386CGate 0x0C // 386 调用门
#define DA_386IGate 0x0E // 386 中断门
#define DA_386TGate 0x0F // 386 陷阱门

// Descriptor Flags
#define DF_G    0x8 // 4KiB 粒度
#define DF_DB   0x4 // 32 位段

// Segment Descriptor
#define SEG_DESC(base, lim, access)                                \
    .word (lim) & 0xFFFF,                                          \
          (base) & 0xFFFF;                                         \
    .byte ((base) >> 16) & 0xFF,                                   \
          (access | DA_P),                                         \
          (((lim) >> 16) & 0x0F) | (((DF_G | DF_DB) << 4) & 0xF0), \
          ((base) >> 24) & 0xFF

#define SEG_NULL    .word 0, 0, 0, 0

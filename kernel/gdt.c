#include "kernel/gdt.h"
#include "string.h"

static SegmentDescriptor gdt[GDT_ENTRY_COUNT];

static void set_gdt_entry(SegmentDescriptor *desc, uint32_t base, uint32_t limit, uint8_t dpl, uint8_t type)
{
    desc->base_low = base & 0xFFFFFF;
    desc->base_hi = (base >> 24) & 0xFF;
    desc->lim_low = limit & 0xFFFF;
    desc->lim_hi = (limit >> 16) & 0xF;
    desc->dpl = dpl;
    desc->type = type;
    desc->accessed = 1;
    desc->present = 1;
    desc->m32 = 1;
    desc->long_mode = 0;
    desc->granularity = 1;
}

void gdt_init(void)
{
    memset(gdt, 0, sizeof(*gdt));                                // NULL Segment
    set_gdt_entry(gdt + KER_CODE_INDEX, 0, 0xFFFFF, 0, DA_CR);   // Kernel Code Segment
    set_gdt_entry(gdt + KER_DATA_INDEX, 0, 0xFFFFF, 0, DA_DRW);  // Kernel Data Segment
    set_gdt_entry(gdt + USER_CODE_INDEX, 0, 0xFFFFF, 3, DA_CR);  // User Code Segment
    set_gdt_entry(gdt + USER_DATA_INDEX, 0, 0xFFFFF, 3, DA_DRW); // User Data Segment

    // Set GDTR
    GDTDescriptor gdtr = {
        .size = sizeof(gdt) - 1,
        .offset = (uint32_t)gdt,
    };

    asm volatile(
        "lgdt %[gdtr]\n"          // 加载 GDT
        "ljmp %[code_seg], $1f\n" // 远跳转刷新 CS 寄存器，加载内核代码段选择子
        "1:"
        "mov %[data_seg], %%ax\n" // 加载内核数据段选择子
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : // 无输出
        : [gdtr] "m"(gdtr),
          [data_seg] "i"(seg_sel_val(KER_DATA_INDEX, 0, 0)),
          [code_seg] "i"(seg_sel_val(KER_CODE_INDEX, 0, 0))
        : "memory", "ax");
}
#include "kernel/x86.h"
#include "kernel/gdt.h"
#include "string.h"

static segment_descriptor _gdt[NR_GDT_ENTRY] = {0};
static tss_struct _tss = {0};

static void set_seg_descriptor(segment_descriptor *desc, uint32_t base, uint32_t limit, uint8_t dpl, uint8_t type)
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

void set_tss(const tss_struct *tss)
{
    _tss = *tss;
}

void set_data_selector(uint16_t selector)
{
    asm volatile(
        "mov %[data_seg], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        :
        : [data_seg] "r"(selector)
        : "memory", "ax");
}

void gdt_init(void)
{
    memset(_gdt, 0, sizeof(*_gdt));                                                // NULL Segment
    set_seg_descriptor(_gdt + KER_CODE_INDEX, 0, 0xFFFFF, 0, DA_CR);               // Kernel Code Segment
    set_seg_descriptor(_gdt + KER_DATA_INDEX, 0, 0xFFFFF, 0, DA_DRW);              // Kernel Data Segment
    set_seg_descriptor(_gdt + USER_CODE_INDEX, 0, 0xFFFFF, 3, DA_CR);              // User Code Segment
    set_seg_descriptor(_gdt + USER_DATA_INDEX, 0, 0xFFFFF, 3, DA_DRW);             // User Data Segment
    set_seg_descriptor(_gdt + GDT_TSS_INDEX, (uint32_t)&_tss, 0xFFFFF, 0, DA_TSS); // TSS

    // Set GDTR
    gdt_descriptor gdtr = {
        .size = sizeof(_gdt) - 1,
        .offset = (uint32_t)_gdt,
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
          [data_seg] "i"(KER_DATA_SELECTOR),
          [code_seg] "i"(KER_CODE_SELECTOR)
        : "memory", "ax");

    // Set TR (TSS)
    ltr(GDT_TSS_SELECTOR);
}
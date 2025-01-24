#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "kernel/memlayout.h"
#include "algobase.h"

static size_t detect_memory(void)
{
    /**
     * 通过 MC146818 RTC 芯片的端口获取内存大小
     * 以下端口是获取地址在 16 MiB 以上的扩展内存大小
     * 数值单位为 64 KiB
     */
    uint32_t low, hi, total;
    outb(0x70, 0x34);
    low = inb(0x71);
    outb(0x70, 0x35);
    hi = inb(0x71);
    total = low | (hi << 8);

    // 内存需要大于 16 MiB
    assert(total != 0);

    // 返回总内存大小（字节）
    return total * 64 * 1024 + (16U << 20);
}

void mem_init(void)
{
    // 检测内存大小
    size_t mem_size = detect_memory();
    DEBUGK("mem_size: %u MiB", mem_size >> 20);

    // 添加内核空间以上的内存到空闲页面记录
    uint32_t kernel_addr_end = *(uint32_t *)(P_KERNEL_ADDR_END + HIGHER_HALF_KERNEL_BASE);
    uint32_t addr = ALIGN_UP(kernel_addr_end, PAGE_SIZE); // 地址进行 4 KiB 对齐
    assert(addr < mem_size);
    pmu_init(addr, (mem_size - addr) / PAGE_SIZE);
}
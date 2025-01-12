#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "boot/args.h"
#include "string.h"
#include "algobase.h"

#define CR0_PG (1 << 31) // CR0 寄存器启用分页功能标志位

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

/**
 * 初始化分页
 * 
 * 创建页目录和页表映射整个内存空间并启用分页功能
 */
static void page_init(size_t mem_size)
{
    // 申请一个页用于存储页目录
    uint32_t page_addr = pmu_alloc();
    assert(page_addr != 0);
    // 初始化页目录
    PageDirEntry *page_dir = (PageDirEntry *)page_addr;
    memset(page_dir, 0, PAGE_SIZE);

    // 创建所有内存地址的页表映射
    for (uint32_t addr = 0; addr < mem_size; addr += PAGE_SIZE)
    {
        // 计算地址对应的页目录下标
        size_t pd_index = addr >> 22;
        // 若不存在页表，则创建
        if (!page_dir[pd_index].present)
        {
            // 申请一个页用于存储页表
            page_addr = pmu_alloc();
            assert(page_addr != 0);
            // 设置页表属性
            page_dir[pd_index].addr = page_addr >> 12;
            page_dir[pd_index].present = 1;
            page_dir[pd_index].us = 0;
            page_dir[pd_index].rw = 1;
            page_dir[pd_index].ps = 0;
        }
        // 找到页表
        PageTabelEntry *page_table = (PageTabelEntry *)(page_dir[pd_index].addr << 12);
        // 计算地址对应的页表下标
        size_t pt_index = (addr >> 12) & 0x3FF;
        // 设置页属性
        page_table[pt_index].addr = addr >> 12;
        page_table[pt_index].present = 1;
        page_table[pt_index].us = 0;
        page_table[pt_index].rw = 1;
    }

    /**
     * 启用分页功能
     * 将页目录地址加载到 CR3 寄存器
     * 并设置 CR0 的 PG 位启用分页功能
     */
    set_cr3((uint32_t)page_dir);
    set_cr0(get_cr0() | CR0_PG);
}

void mem_init(void)
{
    // 检测内存大小
    size_t mem_size = detect_memory();
    DEBUGK("mem_size: %u MiB", mem_size >> 20);

    // 添加内核空间以上的内存到空闲页面记录
    uint32_t kernel_addr_end = *(uint32_t *)P_KERNEL_ADDR_END;
    uint32_t addr = ALIGN_UP(kernel_addr_end, PAGE_SIZE); // 地址进行 4 KiB 对齐
    assert(addr < mem_size);
    pmu_init(addr, (mem_size - addr) / PAGE_SIZE);

    // 初始化内核分页
    // page_init(mem_size);
}
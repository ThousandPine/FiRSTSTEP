#include "kernel/page.h"
#include "kernel/pagemgr.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"

#define CR0_PG (1 << 31) // CR0 寄存器启用分页功能标志位

#define KERNEL_MEM_SIZE (16U << 20) // 为内核保留前 16 MiB 的内存
#define KERNEL_MEM_BASE 0x0U        // 内核保留内存起始地址

#define PT_SIZE 1024                                       // 每个页表包含 1024 个条目
#define PT_COUNT (KERNEL_MEM_SIZE / (PAGE_SIZE * PT_SIZE)) // 页表数量，16 MiB 内存需要 4096 个页表条目，对应 4 张页表和 4 个页目录条目

__attribute__((aligned(4096))) PageTabelEntry pgtabels[PT_COUNT][PT_SIZE] = {0}; // 页表
__attribute__((aligned(4096))) PageDirEntry pgdir[PT_COUNT] = {0};               // 页目录，条目数量等于页表数量

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
    return total * 64 * 1024 + 16 * 1024 * 1024;
}

void page_init()
{
    // 检测内存大小
    size_t mem_size = detect_memory();
    DEBUGK("mem_size: %u MiB", mem_size >> 20);

    assert(mem_size >= KERNEL_MEM_BASE + KERNEL_MEM_SIZE);

    /**
     * 初始化内核页表，映射内核内存空间
     * 在循环中设置页目录和页表映射的地址
     */
    uint32_t addr = KERNEL_MEM_BASE;

    assert((addr & 0xFFF) == 0);            // 物理页地址 4 KiB 对齐
    assert(((uint32_t)pgdir & 0xFFF) == 0); // 页目录地址 4 KiB 对齐

    for (int i = 0; i < PT_COUNT; i++)
    {
        assert(((uint32_t)&pgtabels[i] & 0xFFF) == 0); // 页表地址 4 KiB 对齐

        /**
         * 设置页目录条目，转换为整数直接赋值结构体
         * 高 20 位是页表地址的高 20 位，所以直接将地址按位与 0xFFFFF000
         * 低 12 位是属性，设置为 管理员权限/可读写/存在页面
         */
        *(uint32_t *)(&pgdir[i]) = ((uint32_t)&pgtabels[i] & 0xFFFFF000) | 0x3;

        for (int j = 0; j < PT_SIZE && addr < KERNEL_MEM_BASE + KERNEL_MEM_SIZE; j++)
        {
            // 设置页表条目，原理与上面相同
            *(uint32_t *)(&pgtabels[i][j]) = (addr & 0xFFFFF000) | 0x3;
            addr += PAGE_SIZE;
        }
    }

    /**
     * 启用分页功能
     * 将页目录地址加载到 CR3 寄存器
     * 并设置 CR0 的 PG 位启用分页功能
     */
    set_cr3((uint32_t)pgdir);
    set_cr0(get_cr0() | CR0_PG);

    // 添加内核保留空间以上的内存到空闲页面记录
    pagemgr_add_record(KERNEL_MEM_BASE + KERNEL_MEM_SIZE, (mem_size - KERNEL_MEM_SIZE - KERNEL_MEM_BASE) / PAGE_SIZE);
}
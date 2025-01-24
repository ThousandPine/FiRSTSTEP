#include "kernel/x86.h"
#include "kernel/page.h"
#include "kernel/memlayout.h"
#include "string.h"
#include "kernel/kernel.h"

__attribute__((aligned(4096))) PageDirEntry kernel_page_dir[1024] = {0};           // 内核页目录
__attribute__((aligned(4096))) PageTabelEntry kernel_page_tables[256][1024] = {0}; // 内核页表，256 个页表足以映射 1 GiB 空间

size_t kernel_page_tabel_count = 0; // 使用的内核页表数量

/**
 * 初始化页表映射
 *
 * 创建高段线性地址到内核物理地址的映射，使得后续能通过高段线性地址访问内核空间。
 * 程序目前还运行在低地址上，所以也要创建低段线性地址的映射，防止启用分页后访问不到内核。
 */
__attribute__((section(".lower.text"))) void page_init(void)
{
    // 内核地址范围
    const uint32_t kernel_base = 0;
    const uint32_t kernel_end = *(uint32_t *)P_KERNEL_ADDR_END;

    /**
     * 页目录和页表使用的是物理地址，但 kernel_page_dir 和 kernel_page_tables 
     * 定义在 .bss 段，符号被链接到了高地址，所以要减去偏移量得到实际所在的物理地址
     */
    PageDirEntry *page_dir = (PageDirEntry *)((uint32_t)kernel_page_dir - HIGHER_HALF_KERNEL_BASE);
    PageTabelEntry(*page_tables)[1024] = (PageTabelEntry(*)[1024])((uint32_t)kernel_page_tables - HIGHER_HALF_KERNEL_BASE);

    // 创建低地址的映射，防止启用分页后内存访问出错
    size_t pt_count = 0; // 记录使用的页表数量
    for (uint32_t addr = kernel_base; addr < kernel_end; addr += PAGE_SIZE)
    {
        // 计算地址对应的页目录条目下标
        size_t pd_index = addr >> 22;
        // 若不存在页表记录，则新建
        if (!page_dir[pd_index].present)
        {
            // 记录页表数量
            ++pt_count;
            // 添加到页目录
            page_dir_set(page_dir[pd_index], (uint32_t)page_tables[pt_count - 1], 0, 1, 1);
        }
        // 计算地址对应的页表条目下标
        size_t pt_index = (addr >> 12) & 0x3FF;
        // 设置页属性
        page_tabel_set(page_tables[pt_count - 1][pt_index], addr, 0, 1, 1);
    }

    // 引用之前的页表，创建高段线性地址到内核物理地址的映射
    const size_t pd_hi_index = HIGHER_HALF_KERNEL_BASE >> 22; // 高段内存的页目录条目下标
    for (uint32_t i = 0; i < pt_count; i++)
    {
        page_dir[pd_hi_index + i] = page_dir[i];
    }

    /**
     * 启用分页功能
     * 将页目录地址加载到 CR3 寄存器
     * 并设置 CR0 的 PG 位启用分页功能
     */
    set_cr3((uint32_t)page_dir);
    set_cr0(get_cr0() | CR0_PG);

    /**
     * 记录内核使用的页表数量
     * 该变量保存在 .bss 段，位于高地址，需要在页表初始化后赋值
     */
    kernel_page_tabel_count = pt_count;
}

/**
 * 清除页目录的低段线性地址映射
 */
void clear_lower_page(void)
{
    memset(kernel_page_dir, 0, sizeof(PageDirEntry) * kernel_page_tabel_count);
    set_cr3((uint32_t)kernel_page_dir - HIGHER_HALF_KERNEL_BASE);
}
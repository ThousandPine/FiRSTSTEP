#include "kernel/x86.h"
#include "kernel/page.h"
#include "kernel/memlayout.h"
#include "string.h"
#include "kernel/kernel.h"

__attribute__((aligned(4096))) PageDirEntry kernel_page_dir[1024] = {0};           // 内核页目录
__attribute__((aligned(4096))) PageTabelEntry kernel_page_tables[256][1024] = {0}; // 内核页表，256 个页表足以映射 1 GiB 空间

size_t kernel_page_tabel_count = 0; // 使用的内核页表数量

/**
 * 将内核映射到高段线性地址
 *
 * 先创建页目录让内核地址范围的线性地址与物理地址相同，防止启用分页之后访问不到内核。
 * 然后再创建高段线性地址到内核物理地址的映射（引用刚刚创建的页表即可），使得后续能
 * 通过高段线性地址访问内核空间。
 * 这样一来用户程序就可以使用剩余的低段内存线性地址。
 * 所有用户程序的页目录在低段线性地址维护各自的页表，而高段线性地址共用内核空间的页表。
 * 
 * 该函数用于建立页映射，所以要定义在 .lower.text 段，使链接地址与加载地址相同
 */
__attribute__((section(".lower.text"))) void page_init(void)
{
    // 内核地址范围
    const uint32_t kernel_base = 0;
    const uint32_t kernel_end = *(uint32_t *)P_KERNEL_ADDR_END;

    /**
     * 页目录和页表使用的是物理地址
     * 但 kernel_page_dir 和 kernel_page_tables 定义在 .bss 段，符号被链接到了高地址
     * 要减去偏移量得到实际所在的物理地址
     */
    PageDirEntry *page_dir = (PageDirEntry *)((uint32_t)kernel_page_dir - HIGHER_HALF_KERNEL_BASE);
    PageTabelEntry(*page_tables)[1024] = (PageTabelEntry(*)[1024])((uint32_t)kernel_page_tables - HIGHER_HALF_KERNEL_BASE);

    // 创建内核原地址的映射，防止启用分页后内存访问出错
    size_t pt_count = 0; // 使用的页表数量
    for (uint32_t addr = kernel_base; addr < kernel_end; addr += PAGE_SIZE)
    {
        // 计算地址对应的页目录下标
        size_t pd_index = addr >> 22;
        // 若不存在页表记录，则设置
        if (!page_dir[pd_index].present)
        {
            // 记录创建的页表下标
            ++pt_count;
            // 设置页表属性
            page_dir_set(page_dir[pd_index], (uint32_t)page_tables[pt_count - 1], 0, 1, 1);
        }
        // 计算地址对应的页表下标
        size_t pt_index = (addr >> 12) & 0x3FF;
        // 设置页属性
        page_tabel_set(page_tables[pt_count - 1][pt_index], addr, 0, 1, 1);
    }

    // 引用之前的页表，创建高段线性地址到内核地址的映射
    const size_t pd_hi_index = HIGHER_HALF_KERNEL_BASE >> 22; // 高段内存的页目录下标
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
     * 该变量保存在 .bss 段，位于高地址
     * 所以要在页表初始化后赋值
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
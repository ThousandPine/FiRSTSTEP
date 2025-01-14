#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "kernel/memlayout.h"
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
 * 将内核映射到高段内存空间
 *
 * 先创建页目录让内核地址范围的线性地址与物理地址相同，防止启用分页之后访问不到内核。
 * 然后再创建高段线性地址到内核物理地址的映射（引用刚刚创建的页表即可），使得后续能
 * 通过高段线性地址访问内核空间。
 * 接着启用分页功能，并初始化 GDT
 * 这样一来用户程序就可以使用剩余的低段内存线性地址。
 * 所有用户程序的页目录在低段线性地址维护各自的页表，而高段线性地址共用内核空间的页表。
 *
 * 用户程序页表映射内核空间的好处是，从用户态到内核态时可以直接访问到内核空间，而不用
 * 切换到“内核页目录”。
 * 如果用户程序页表独立于内核，那么通过中断切换到内核态时就会因为页表没有切换导致错误，
 * 除非中断调用的是“任务门”，会自动修改为指定的 CR3 寄存器值。但任务门的缺点是执行慢，
 * 且不可重入。也就是说调用任务门之后要关中断，在灵活性上也打了折扣。
 * https://stackoverflow.com/questions/53537289/what-page-directory-is-used-during-an-x86-interrupt-handler
 * https://wiki.osdev.org/Interrupt_Descriptor_Table#Task_Gate
 */
static void higher_half_kernel(uint32_t kernel_base, uint32_t kernel_end)
{
    // 申请一个页用于存储页目录
    uint32_t page_addr = pmu_alloc();
    assert(page_addr != 0);
    // 初始化页目录
    PageDirEntry *const page_dir = (PageDirEntry *)page_addr;
    memset(page_dir, 0, PAGE_SIZE);

    /**
     * 创建内核原地址的映射
     * 因为还没通过分段将内核映射到高内存，先创建内核原地址的映射防止启用分页后访问出错
     */
    size_t pt_first = __UINT32_MAX__, pt_last = 0;
    for (uint32_t addr = kernel_base; addr < kernel_end; addr += PAGE_SIZE)
    {
        // 计算地址对应的页目录下标
        size_t pd_index = addr >> 22;
        // 若不存在页表，则创建
        if (!page_dir[pd_index].present)
        {
            // 记录创建的页表下标范围
            pt_first = MIN(pt_first, pd_index);
            pt_last = MAX(pt_last, pd_index);
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
     * 只需要引用之前的页表，创建高段线性地址到内核物理地址的映射
     * 选用最后几个页表的位置表作为高段地址
     */
    const size_t pt_num = pt_last - pt_first + 1; // 页表数量
    const size_t pt_hi_index = 1024 - pt_num;     // 高段内存的页表下标
    for (uint32_t i = 0; i < pt_num; i++)
    {
        page_dir[pt_hi_index + i] = page_dir[pt_first + i];
    }

    /**
     * 启用分页功能
     * 将页目录地址加载到 CR3 寄存器
     * 并设置 CR0 的 PG 位启用分页功能
     */
    set_cr3((uint32_t)page_dir);
    set_cr0(get_cr0() | CR0_PG);

    gdt_init();
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

    // 初始化内核分页
    higher_half_kernel(0, kernel_addr_end);
}
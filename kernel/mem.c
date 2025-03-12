#include "kernel/gdt.h"
#include "kernel/page.h"
#include "kernel/pmu.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "boot/args.h"
#include "string.h"
#include "algobase.h"

static page_dir_entry kernel_page_dir[1024] __attribute__((aligned(PAGE_SIZE))) = {0};
uint32_t kernel_area_page_dir_end_index = __UINT32_MAX__; // 在此之前的页表为内核专用区域，用户页表也要映射这些页表


/**
 * 映射物理地址到指定线性地址
 * 
 * @return 0 表示成功，-1 表示失败
 */
int map_physical_page_to_linear(page_dir_entry *page_dir, uint32_t phys_addr, uint32_t linear_addr, uint8_t us, uint8_t rw)
{
    assert(page_dir != NULL);
    assert(phys_addr != 0);
    assert(linear_addr != 0);

    uint32_t pd_index = page_dir_index(linear_addr);
    uint32_t pt_index = page_table_index(linear_addr);

    if (pd_index < kernel_area_page_dir_end_index)
    {
        panic("Can't map page to kernel area %p", linear_addr);
    }

    if (!page_dir[pd_index].present)
    {
        uint32_t page_table_addr = pmu_alloc();
        assert(page_table_addr != 0);
        memset((void *)(page_table_addr), 0, PAGE_SIZE);
        page_dir[pd_index].addr = page_table_addr >> 12;
        page_dir[pd_index].present = 1;
        page_dir[pd_index].us = us;
        page_dir[pd_index].rw = 1; // 页目录始终读写，具体权限由页表控制
    }
    page_tabel_entry *page_table = (page_tabel_entry *)(page_dir[pd_index].addr << 12);
    // 线性地址已被映射则返回错误
    if (page_table[pt_index].present)
    {
        panic("Linear address %p has been mapped", linear_addr);
        return -1;
    }
    page_table[pt_index].addr = phys_addr >> 12;
    page_table[pt_index].present = 1;
    page_table[pt_index].us = us;
    page_table[pt_index].rw = rw;
    return 0;
}

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
 * 初始化内核页表
 * 
 * 创建页目录和页表映射整个内存空间，便于内核管理内存
 */
static void kernel_page_init(size_t mem_size)
{
    // 初始化页目录
    memset(kernel_page_dir, 0, PAGE_SIZE);

    // 创建所有内存地址的页表映射
    for (uint32_t addr = 0; addr < mem_size; addr += PAGE_SIZE)
    {
        // 计算地址对应的页目录下标
        size_t pd_index = page_dir_index(addr);
        // 若不存在页表，则创建
        if (!kernel_page_dir[pd_index].present)
        {
            // 申请一个页用于存储页表
            uint32_t page_addr = pmu_alloc();
            assert(page_addr != 0);
            // 设置页表属性
            kernel_page_dir[pd_index].addr = page_addr >> 12;
            kernel_page_dir[pd_index].present = 1;
            kernel_page_dir[pd_index].us = 0;
            kernel_page_dir[pd_index].rw = 1;
            kernel_page_dir[pd_index].ps = 0;
        }
        // 找到页表
        page_tabel_entry *page_table = (page_tabel_entry *)(kernel_page_dir[pd_index].addr << 12);
        // 计算地址对应的页表下标
        size_t pt_index = page_table_index(addr);
        // 设置页属性
        page_table[pt_index].addr = addr >> 12;
        page_table[pt_index].present = 1;
        page_table[pt_index].us = 0;
        page_table[pt_index].rw = 1;
    }
}

/**
 * 启用分页功能
 */
static void page_enable(void)
{
    // 将页目录地址加载到 CR3 寄存器
    set_cr3((uint32_t)kernel_page_dir);
    // 设置 CR0 的 PG 位启用分页功能
    set_cr0(get_cr0() | CR0_PG);
}

/**
 * 初始化内核分页
 */
static void page_init(size_t mem_size)
{
    uint32_t kernel_addr_end = *(uint32_t *)P_KERNEL_ADDR_END;
    kernel_area_page_dir_end_index = page_dir_index(kernel_addr_end) + !!page_table_index(kernel_addr_end);
    kernel_page_init(mem_size);
    page_enable();
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
    page_init(mem_size);
}
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"

size_t detect_memory(void)
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
    DEBUGK("mem_size = %u MiB", mem_size >> 20);

    // 初始化内核页表（映射保留空间）

    // 启用分页功能

    // 初始化空闲页记录
}
#include "x86.h"
#include "mbr.h"

#define SECTSIZE 512            // 扇区大小为 512 字节
#define MBR (((struct MBR *) 0x7C00))   // 指针类型转换，读取位于内存 0x7C00 的 MBR

char *vmem = (char *)0xB8000;   // 显存指针
char vattr = 0x02;              // 默认显示绿色字符

void putchar(char c);
void puts(char *s);
void putn(uint32_t n);
void error(char *s);
void waitdisk(void);
void readsect(void *dst, uint32_t offset);

void print_part(struct PartitionEntry const *part)
{
    puts(" start_lba:");
    putn(part->start_lba);
    puts(" num_sectors:");
    putn(part->num_sectors);
}

void setupmain(void)
{
    puts("In protected mode.");

    struct PartitionEntry const * boot_part = NULL;

    // 找到首个可引导分区
    for (uint32_t i = 0; i < 4; i++)
    {
        if (MBR->partitions[i].boot_indicator == 0x80)
        {
            boot_part = &MBR->partitions[i];
            break;
        }
    }

    if (boot_part == NULL)
    {
        error("No bootable partition found");
    }

    print_part(boot_part);

    // 寻找 kernel 程序



    puts("DONE.");
    while (1)
        ;
bad:
    error("An error occurred during setup!");
}

void putchar(char c)
{
    *(vmem++) = c;
    *(vmem++) = vattr;
}

void puts(char *s)
{
    while (*s)
    {
        putchar(*(s++));
    }
}

void putn(uint32_t n)
{
    uint32_t x = 0;
    while (n)
    {
        x *= 10;
        x += n % 10;
        n /= 10;
    }
    while (x)
    {
        putchar((x % 10) + '0');
        x /= 10;
    }
}

void error(char *s)
{
    vattr = 0xC; // 显示红色高亮字符
    puts(s);
    while (1)
        /* do nothing */;
}

void waitdisk(void)
{
    // 等待硬盘就绪
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* do nothing */;
}

void readsect(void *dst, uint32_t offset)
{
    if (offset > 0x0FFFFFFF)
    {
        error("Offset exceeds LBA28 boundary");
    }

    waitdisk();

    outb(0x1F2, 1); // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20); // cmd 0x20 - read sectors

    waitdisk();

    // 读取扇区
    insl(0x1F0, dst, SECTSIZE / 4);
}
#include "x86.h"

#define SECTSIZE 512

char *vmem = (char *)0xB8000;
char vattr = 0x02; // 绿色字符

void putchar(char c);
void puts(char *s);
void error(char *s);
void waitdisk(void);
void readsect(void *dst, uint32_t offset);

void setupmain(void)
{
    puts("In protected mode.");

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

void error(char *s)
{
    vattr = 0xC; // 高亮红色字符
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
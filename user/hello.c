/* 用户程序测试代码 */
#include "string.h"

char *vmem = (char *)0xB8000;   // 显存指针

void putchar(char c);
void puts(char *s);

void main(void)
{
    puts("Hello World!!!");
    strlen("123");
    while(1);
}

void putchar(char c)
{
    *(vmem++) = c;
    *(vmem++) = 0x9; // 高亮紫色字符
}

void puts(char *s)
{
    while (*s)
    {
        putchar(*(s++));
    }
}
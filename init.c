/* 内核测试代码 */

char *vmem = (char *)0xB8000;   // 显存指针

void putchar(char c);
void puts(char *s);

void setupmain(void)
{
    puts("In Kernel Init!!!");
    while(1);
}

void putchar(char c)
{
    *(vmem++) = c;
    *(vmem++) = 0x9; // 高亮蓝色字符
}

void puts(char *s)
{
    while (*s)
    {
        putchar(*(s++));
    }
}
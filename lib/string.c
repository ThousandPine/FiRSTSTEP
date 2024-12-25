#include "types.h"

size_t strlen(const char *str)
{
    size_t i = 0;
    while (str[i++])
        ;
    return i;
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int strncmp(const char *str1, const char *str2, size_t n)
{
    if (n == 0)
    {
        return 0; // 比较 0 个字符时，总是相等
    }

    while (n-- && *str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

char *strcpy(char *dest, const char *src)
{
    char *original_dest = dest;
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

/**
 * NOTE: 源字符串长度（不包含 '\0'）大于等于 n 则不会添加 '\0'
 */
char *strncpy(char *dest, const char *src, size_t n)
{
    char *original_dest = dest;

    while (n && *src)
    {
        *dest++ = *src++;
        n--;
    }

    // 如果源字符串长度小于 n，填充目标字符串剩余部分为 '\0'
    // 反之源字符串长度（不包含 '\0'）大于等于 n，则不会添加 '\0'
    while (n--)
    {
        *dest++ = '\0';
    }

    return original_dest;
}

char *strcat(char *dest, const char *src)
{
    char *original_dest = dest;
    // 找到目标字符串的末尾
    while (*dest)
    {
        dest++;
    }
    // 将源字符串复制到目标字符串的末尾
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

/**
 * NOTE: 算上结尾 '\0' 实际上会修改 n + 1 个字节
 */
char *strncat(char *dest, const char *src, size_t n)
{
    char *original_dest = dest;
    // 找到目标字符串的末尾
    while (*dest)
    {
        dest++;
    }
    // 将源字符串复制到目标字符串的末尾
    while (n-- && *src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = s1, *p2 = s2;
    while (n-- && *p1 == *p2)
    {
        p1++, p2++;
    }
    return *p1 - *p2;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--)
    {
        *d++ = *s++;
    }
    return dest;
}

/**
 * @param c 填充数据，只会使用最低 1 字节的内容
 */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = s;
    while (n--)
    {
        *p++ = (uint8_t)c;
    }
    return s;
}
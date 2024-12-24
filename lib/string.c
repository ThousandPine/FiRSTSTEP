#include "types.h"

size_t strlen(const char *str)
{
    int i = 0;
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
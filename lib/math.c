
#include "types.h"

/**
 * 手动实现无符号 64 位整数取模
 *
 * 无符号 64 位整数取模运算需要定义特殊函数 __umoddi3
 * 由于没有链接标准库，所以只能手动实现这两个方法
 *
 * FIXME: 没有实现 base 为 0 的异常触发
 */
uint64_t __umoddi3(uint64_t n, uint64_t base)
{
    if (base == 0)
    {
        // 处理除零错误，通常硬件会触发异常，但这里我们简单返回 0
        return 0;
    }

    // 如果 n 小于 base，则直接返回 n
    if (n < base)
    {
        return n;
    }

    // 手动模拟 64 位整数除法的取模运算
    uint64_t result = 0;
    for (int i = 63; i >= 0; i--)
    {
        // 将 n 的每一位依次加入 result
        result = (result << 1) | ((n >> i) & 1);

        // 如果当前 result 大于等于 base，则减去 base
        if (result >= base)
        {
            result -= base;
        }
    }

    return result;
}

/**
 * 手动实现无符号 64 位整数除法
 *
 * 无符号 64 位整数除法运算需要定义特殊函数 __udivdi3
 * 由于没有链接标准库，所以只能手动实现这两个方法
 *
 * FIXME: 没有实现 base 为 0 的异常触发
 */
uint64_t __udivdi3(uint64_t n, uint64_t base)
{
    if (base == 0)
    {
        // 处理除零错误，通常硬件会触发异常，但这里我们简单返回最大值
        return ~0ULL; // 无符号整数的最大值
    }

    if (n < base)
    {
        // 如果 n 小于 base，则商为 0
        return 0;
    }

    uint64_t result = 0; // 商
    uint64_t remain = 0; // 余数

    // 从高位到低位逐位处理 n
    for (int i = 63; i >= 0; i--)
    {
        // 将 n 的每一位依次加入 remain
        remain = (remain << 1) | ((n >> i) & 1);

        // 如果当前 remain 大于等于 base，则减去 base，并在商的相应位上设为 1
        if (remain >= base)
        {
            remain -= base;
            result |= (1ULL << i);
        }
    }

    return result;
}
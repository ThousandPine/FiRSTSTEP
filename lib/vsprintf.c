#include "varg.h"

#define is_dight(x) ((x) >= '0' && (x) <= '9')

#define LEFT (1 << 0)    // 字段宽度内左对齐
#define PLUS (1 << 1)    // 正数显示加号
#define SPACE (1 << 2)   // 如果没有输出符号，则用空格代替（使用 PLUS 后则无效）
#define SPEC (1 << 3)    // 输出 0, 0x 前缀; 浮点数强制显示小数点
#define ZEROPAD (1 << 4) // 左侧填充 0 而不是空格
#define SIGNED (1 << 5)  // 有符号数

// 字符串转换为数字，并跳过这段字符
static int skip_atoi(const char **s)
{
    int i = 0;
    while (is_dight(**s))
    {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

static int64_t get_int(va_list *args, int lprefix, int hprefix)
{
    if (lprefix >= 2)
    {
        return va_arg(*args, long long);
    }
    if (lprefix == 1)
    {
        return va_arg(*args, long);
    }
    if (hprefix >= 2)
    {
        return va_arg(*args, char);
    }
    if (hprefix == 1)
    {
        return va_arg(*args, short);
    }
    return va_arg(*args, int);
}

static uint64_t get_uint(va_list *args, int lprefix, int hprefix)
{
    if (lprefix >= 2)
    {
        return va_arg(*args, unsigned long long);
    }
    if (lprefix == 1)
    {
        return va_arg(*args, unsigned long);
    }
    if (hprefix >= 2)
    {
        return va_arg(*args, unsigned char);
    }
    if (hprefix == 1)
    {
        return va_arg(*args, unsigned short);
    }
    return va_arg(*args, unsigned int);
}

// 翻转无符号数并返回位数
static int reverse_uint(uint64_t *num, uint64_t base)
{
    int t = 0;
    uint64_t x = 0;
    do
    {
        t++;
        x *= base;
        x += *num % base;
        *num /= base;
    } while (*num);
    *num = x;
    return t;
}

// 数字数格式化为字符串
static int number(char *buf, uint64_t num, uint64_t base, int flags, int field_width, int precision)
{
    int len = 0;

    // 处理有符号数
    if (flags & SIGNED)
    {
        // 处理负号，转为无符号数处理
        if ((int64_t)num < 0)
        {
            num = -(int64_t)num;
            buf[len++] = '-';
        }
        // 正数输出加号
        else if (flags & PLUS)
        {
            buf[len++] = '+';
        }
        // 空格占位符号位
        else if (flags & SPACE)
        {
            buf[len++] = ' ';
        }
    }

    // 输出特殊前缀
    if (flags & SPEC)
    {
        if (base == 8)
        {
            buf[len++] = '0';
        }
        else if (base == 16)
        {
            buf[len++] = '0';
            buf[len++] = 'x';
        }
    }

    // 反转数字并计算位数
    int t = reverse_uint(&num, base);

    // 数字位数少于精度时，在开头用 0 补充
    for (int i = t; i < precision; i++)
    {
        buf[len++] = '0';
    }

    // 输出数字
    while (t--)
    {
        buf[len++] = "0123456789ABCDEF"[num % base];
        num /= base;
    }

    // 填充宽度
    if (len < field_width)
    {
        const char fillc = (flags & ZEROPAD) ? '0' : ' ';

        // 左对齐
        if (flags & LEFT)
        {
            // 填充末尾
            while (len < field_width)
            {
                buf[len++] = fillc;
            }
        }
        // 右对齐
        else
        {
            // 右移内容
            for (int i = len - 1; i >= 0; i--)
            {
                buf[i + field_width - len] = buf[i];
            }
            // 填充开头
            for (int i = 0; i < field_width - len; i++)
            {
                buf[i] = fillc;
            }
            len = field_width;
        }
    }

    return len;
}

/**
 * 根据参数格式化字符串
 *
 * 会在结尾添加 '\0'
 *
 * FIXME: 不支持浮点数和部分类型前缀
 * FIXME: 可能超过 buf 边界
 *
 * @param buf 结果字符串保存地址
 * @param fmt 格式字符串
 * @param args 参数列表
 * @return 返回格式化后的字符串长度
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
    int len = 0;

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            buf[len++] = *fmt;
            continue;
        }

        // flags
        int flags = 0;
    repeat:
        fmt++;
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPEC;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;

        default:
            break;
        }

        // 填充宽度
        int field_width = 0;
        if (is_dight(*fmt))
        {
            field_width = skip_atoi(&fmt);
        }
        else if (*fmt == '*')
        {
            fmt++;
            // * 表示从参数中获取宽度
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                // 负数表示靠左（等价于 flags 的 '-'）
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // 精度
        int precision = 0;
        if (*fmt == '.')
        {
            fmt++;
            if (is_dight(*fmt))
            {
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == '*')
            {
                fmt++;
                precision = va_arg(args, int);
                precision = precision > 0 ? precision : 0;
            }
        }

        // 识别类型前缀字符
        int lprefix = 0; // 前缀 l 数量
        int hprefix = 0; // 前缀 h 数量
        for (; *fmt == 'l' || *fmt == 'h'; fmt++)
        {
            lprefix += (*fmt == 'l');
            hprefix += (*fmt == 'h');
        }


        // 识别类型并格式化字符
        switch (*fmt)
        {
        // 整数
        case 'i':
        case 'd':
            uint64_t num = 0;
            flags |= SIGNED;                        // 符号标记
            num = get_int(&args, lprefix, hprefix); // 要用 get_int 读取有符号数
            len += number(buf + len, num, 10, flags, field_width, precision);
            break;
        case 'u':
            num = get_uint(&args, lprefix, hprefix);
            len += number(buf + len, num, 10, flags, field_width, precision);
            break;
        case 'o':
            num = get_uint(&args, lprefix, hprefix);
            len += number(buf + len, num, 8, flags, field_width, precision);
            break;
        case 'x':
        case 'X':
            num = get_uint(&args, lprefix, hprefix);
            len += number(buf + len, num, 16, flags, field_width, precision);
            break;

        // 指针
        case 'p':
            flags |= SPEC;                                             // 默认输出 0x 前缀
            precision = precision ? precision : (sizeof(void *) << 1); // 默认精度为指针位宽
            num = (uint32_t)va_arg(args, void *);
            len += number(buf + len, num, 16, flags, field_width, precision);
            break;

        // 字符
        case 'c':
            buf[len++] = va_arg(args, char);
            break;

        // 字符串
        case 's':
            char *s = va_arg(args, char *);
            while (*s)
            {
                buf[len++] = *s++;
            }
            break;

        // 将当前长度值写入到参数提供的地址
        case 'n':
            int *intp = va_arg(args, int *);
            *intp = len;
            break;

        // 转义 %
        case '%':
            buf[len++] = '%';
            break;

        // 不支持类型，输出原内容
        default:
            buf[len++] = '%';
            buf[len++] = *fmt;
            break;
        }
    }
    buf[len] = '\0';
    return len;
}

int sprintf(char *buf, const char *fmt, ...)
{
    int n = 0;
    va_list args = va_start(fmt);
    n = vsprintf(buf, fmt, args);
    va_end(args);
    return n;
}